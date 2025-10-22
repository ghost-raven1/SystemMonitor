// linux_hwmon.c - Linux hwmon interface для температур и вентиляторов
#include "platform/linux_hwmon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define HWMON_BASE_PATH "/sys/class/hwmon"
#define THERMAL_BASE_PATH "/sys/class/thermal"
#define MAX_PATH_LEN 256
#define MAX_NAME_LEN 64

// Чтение значения из файла в sysfs
static int read_sysfs_value(const char *path, float *value) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    
    char buffer[32];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    *value = atof(buffer) / 1000.0f; // Большинство датчиков возвращают значения в миллиградусах
    return 0;
}

// Поиск hwmon устройств
static int find_hwmon_devices(char devices[][MAX_PATH_LEN], int max_devices) {
    DIR *dir = opendir(HWMON_BASE_PATH);
    if (!dir) return 0;
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL && count < max_devices) {
        if (entry->d_type == DT_LNK && entry->d_name[0] != '.') {
            snprintf(devices[count], MAX_PATH_LEN, "%s/%s", HWMON_BASE_PATH, entry->d_name);
            count++;
        }
    }
    closedir(dir);
    return count;
}

// Получение температуры CPU через hwmon
float get_cpu_temperature(void) {
    char devices[16][MAX_PATH_LEN];
    int device_count = find_hwmon_devices(devices, 16);
    
    for (int i = 0; i < device_count; i++) {
        char name_path[MAX_PATH_LEN];
        char temp_path[MAX_PATH_LEN];
        
        snprintf(name_path, MAX_PATH_LEN, "%s/name", devices[i]);
        
        FILE *fp = fopen(name_path, "r");
        if (fp) {
            char name[MAX_NAME_LEN];
            if (fgets(name, sizeof(name), fp)) {
                if (strstr(name, "coretemp") != NULL || 
                    strstr(name, "k10temp") != NULL ||
                    strstr(name, "cpu-thermal") != NULL) {
                    fclose(fp);
                    
                    // Ищем temp1_input файл
                    for (int j = 1; j <= 8; j++) {
                        snprintf(temp_path, MAX_PATH_LEN, "%s/temp%d_input", devices[i], j);
                        float temp;
                        if (read_sysfs_value(temp_path, &temp) == 0) {
                            return temp;
                        }
                    }
                }
            }
            fclose(fp);
        }
    }
    
    return 50.0f; // Fallback температура
}

// Получение температур ядер CPU
int linux_get_core_temperatures(float *out, int max, int *written) {
    if (!out || max <= 0 || !written) return -1;
    
    *written = 0;
    char devices[16][MAX_PATH_LEN];
    int device_count = find_hwmon_devices(devices, 16);
    
    for (int i = 0; i < device_count; i++) {
        char name_path[MAX_PATH_LEN];
        snprintf(name_path, MAX_PATH_LEN, "%s/name", devices[i]);
        
        FILE *fp = fopen(name_path, "r");
        if (fp) {
            char name[MAX_NAME_LEN];
            if (fgets(name, sizeof(name), fp)) {
                if (strstr(name, "coretemp") != NULL) {
                    fclose(fp);
                    
                    // Ищем все temp*_input файлы для ядер
                    for (int j = 1; j <= max && j <= 8; j++) {
                        char temp_path[MAX_PATH_LEN];
                        snprintf(temp_path, MAX_PATH_LEN, "%s/temp%d_input", devices[i], j);
                        
                        float temp;
                        if (read_sysfs_value(temp_path, &temp) == 0 && *written < max) {
                            out[*written] = temp;
                            (*written)++;
                        }
                    }
                    return 0;
                }
            }
            fclose(fp);
        }
    }
    
    return -1;
}

// Получение температуры памяти
int linux_get_mem_temperature(float *temp) {
    if (!temp) return -1;
    
    // Попробуем найти через thermal зоны
    DIR *dir = opendir(THERMAL_BASE_PATH);
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
            char type_path[MAX_PATH_LEN];
            char temp_path[MAX_PATH_LEN];
            
            snprintf(type_path, MAX_PATH_LEN, "%s/%s/type", THERMAL_BASE_PATH, entry->d_name);
            snprintf(temp_path, MAX_PATH_LEN, "%s/%s/temp", THERMAL_BASE_PATH, entry->d_name);
            
            FILE *fp = fopen(type_path, "r");
            if (fp) {
                char type[MAX_NAME_LEN];
                if (fgets(type, sizeof(type), fp) && 
                    (strstr(type, "memory") != NULL || strstr(type, "ddr") != NULL)) {
                    fclose(fp);
                    closedir(dir);
                    
                    if (read_sysfs_value(temp_path, temp) == 0) {
                        return 0;
                    }
                }
                fclose(fp);
            }
        }
    }
    closedir(dir);
    
    *temp = -1.0f;
    return -1;
}

// Получение скорости вентиляторов
float get_fan_speed(void) {
    char devices[16][MAX_PATH_LEN];
    int device_count = find_hwmon_devices(devices, 16);
    
    for (int i = 0; i < device_count; i++) {
        // Ищем fan1_input файл
        char fan_path[MAX_PATH_LEN];
        snprintf(fan_path, MAX_PATH_LEN, "%s/fan1_input", devices[i]);
        
        float rpm;
        if (read_sysfs_value(fan_path, &rpm) == 0) {
            return rpm;
        }
    }
    
    return 1200.0f; // Fallback скорость
}

// Обертки для совместимости с macOS API
int smc_get_core_temperatures(float *out, int max, int *written) {
    return linux_get_core_temperatures(out, max, written);
}

int smc_get_mem_temperature(float *temp) {
    return linux_get_mem_temperature(temp);
}
