// usb_linux.c - Linux USB support через /proc и sysfs
#include "platform/usb.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define USB_SYSFS_PATH "/sys/bus/usb/devices"
#define MAX_PATH_LEN 256

// Получение информации об USB устройстве из sysfs
static int read_usb_device_info(const char *device_path, usb_device_t *device) {
    char product_path[MAX_PATH_LEN];
    char manufacturer_path[MAX_PATH_LEN];
    
    snprintf(product_path, sizeof(product_path), "%s/product", device_path);
    snprintf(manufacturer_path, sizeof(manufacturer_path), "%s/manufacturer", device_path);
    
    // Читаем название продукта
    FILE *fp = fopen(product_path, "r");
    if (fp) {
        if (fgets(device->name, sizeof(device->name), fp)) {
            // Убираем перенос строки
            size_t len = strlen(device->name);
            if (len > 0 && device->name[len-1] == '\n') {
                device->name[len-1] = '\0';
            }
        }
        fclose(fp);
    }
    
    // Если нет product, попробуем manufacturer
    if (device->name[0] == '\0') {
        fp = fopen(manufacturer_path, "r");
        if (fp) {
            char manufacturer[128];
            if (fgets(manufacturer, sizeof(manufacturer), fp)) {
                size_t len = strlen(manufacturer);
                if (len > 0 && manufacturer[len-1] == '\n') {
                    manufacturer[len-1] = '\0';
                }
                snprintf(device->name, sizeof(device->name), "%s Device", manufacturer);
            }
            fclose(fp);
        }
    }
    
    // Если все еще нет имени, используем generic
    if (device->name[0] == '\0') {
        strncpy(device->name, "USB Device", sizeof(device->name) - 1);
    }
    
    strncpy(device->type, "USB", sizeof(device->type) - 1);
    device->speed_mbps = 0.0f;
    
    return 0;
}

int list_usb_devices(usb_device_t *out, int max, int *count) {
    if (!out || max <= 0 || !count) return -1;
    
    *count = 0;
    
    DIR *dir = opendir(USB_SYSFS_PATH);
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < max) {
        // Пропускаем служебные файлы и папки
        if (entry->d_name[0] == '.' || entry->d_type != DT_LNK) {
            continue;
        }
        
        // Проверяем, что это USB устройство (содержит : в названии)
        if (strchr(entry->d_name, ':') == NULL) {
            continue;
        }
        
        char device_path[MAX_PATH_LEN];
        snprintf(device_path, sizeof(device_path), "%s/%s", USB_SYSFS_PATH, entry->d_name);
        
        // Проверяем, что device существует и это USB девайс
        char class_path[MAX_PATH_LEN];
        snprintf(class_path, sizeof(class_path), "%s/bDeviceClass", device_path);
        
        FILE *fp = fopen(class_path, "r");
        if (fp) {
            // Если можем прочитать bDeviceClass, значит это USB устройство
            fclose(fp);
            
            if (read_usb_device_info(device_path, &out[*count]) == 0) {
                (*count)++;
            }
        }
    }
    
    closedir(dir);
    return 0;
}
