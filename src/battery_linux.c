// battery_linux.c - Linux battery support через sysfs
#include "battery.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define POWER_SUPPLY_PATH "/sys/class/power_supply"
#define MAX_PATH_LEN 256

// Чтение строки из sysfs файла
static int read_sysfs_string(const char *path, char *buffer, size_t buffer_size) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    
    if (!fgets(buffer, buffer_size, fp)) {
        fclose(fp);
        return -1;
    }
    
    // Убираем перенос строки
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    fclose(fp);
    return 0;
}

// Чтение числового значения из sysfs файла
static int read_sysfs_int(const char *path, int *value) {
    char buffer[32];
    if (read_sysfs_string(path, buffer, sizeof(buffer)) != 0) {
        return -1;
    }
    
    *value = atoi(buffer);
    return 0;
}

// Поиск батареи в sysfs
static int find_battery_path(char *battery_path, size_t path_size) {
    DIR *dir = opendir(POWER_SUPPLY_PATH);
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_LNK && strncmp(entry->d_name, "BAT", 3) == 0) {
            snprintf(battery_path, path_size, "%s/%s", POWER_SUPPLY_PATH, entry->d_name);
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return -1;
}

int get_battery_info(battery_info_t *out) {
    if (!out) return -1;
    
    out->percentage = -1.0f;
    out->charging = -1;
    out->time_remaining_min = -1;
    
    // Quick exit в safe mode
    const char *safe = getenv("SYSMON_SAFE");
    if (safe && safe[0] == '1') return 0;
    
    // Respect explicit disable
    const char *no_bat = getenv("SYSMON_NO_BAT");
    if (no_bat && no_bat[0] == '1') return 0;
    
    char battery_path[MAX_PATH_LEN];
    if (find_battery_path(battery_path, sizeof(battery_path)) != 0) {
        return -1; // Батарея не найдена (desktop система)
    }
    
    // Читаем процент заряда
    char capacity_path[MAX_PATH_LEN];
    snprintf(capacity_path, sizeof(capacity_path), "%s/capacity", battery_path);
    int capacity;
    if (read_sysfs_int(capacity_path, &capacity) == 0) {
        out->percentage = (float)capacity;
    }
    
    // Читаем статус зарядки
    char status_path[MAX_PATH_LEN];
    snprintf(status_path, sizeof(status_path), "%s/status", battery_path);
    char status[32];
    if (read_sysfs_string(status_path, status, sizeof(status)) == 0) {
        if (strcmp(status, "Charging") == 0) {
            out->charging = 1;
        } else if (strcmp(status, "Discharging") == 0) {
            out->charging = 0;
        }
    }
    
    // Читаем оставшееся время (если доступно)
    if (out->charging == 0) { // Только при разрядке
        char time_to_empty_path[MAX_PATH_LEN];
        snprintf(time_to_empty_path, sizeof(time_to_empty_path), "%s/time_to_empty", battery_path);
        int time_to_empty;
        if (read_sysfs_int(time_to_empty_path, &time_to_empty) == 0) {
            out->time_remaining_min = time_to_empty / 60; // конвертируем секунды в минуты
        }
    }
    
    return 0;
}

int get_battery_cycle_count(int *cycles) {
    if (!cycles) return -1;
    
    char battery_path[MAX_PATH_LEN];
    if (find_battery_path(battery_path, sizeof(battery_path)) != 0) {
        return -1;
    }
    
    char cycle_count_path[MAX_PATH_LEN];
    snprintf(cycle_count_path, sizeof(cycle_count_path), "%s/cycle_count", battery_path);
    
    return read_sysfs_int(cycle_count_path, cycles);
}
