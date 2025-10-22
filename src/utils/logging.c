// logging.c
#include "utils/logging.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

// Функция безопасной валидации пути лог файла
static int is_safe_log_path(const char *path) {
    if (!path || strlen(path) == 0 || strlen(path) >= PATH_MAX) {
        return 0;
    }

    // Проверяем на опасные последовательности
    if (strstr(path, "..") != NULL) {
        return 0;
    }

    // Проверяем что путь начинается с разрешенного префикса
    const char *safe_prefixes[] = {"./", "src/", NULL};
    int is_safe = 0;

    for (int i = 0; safe_prefixes[i] != NULL; i++) {
        if (strncmp(path, safe_prefixes[i], strlen(safe_prefixes[i])) == 0) {
            is_safe = 1;
            break;
        }
    }

    if (!is_safe) {
        return 0;
    }

    // Разрешаем файлы с расширениями .log и .csv
    const char *ext = strrchr(path, '.');
    if (!ext || (strcmp(ext, ".log") != 0 && strcmp(ext, ".csv") != 0)) {
        return 0;
    }

    return 1;
}

void log_info(const char *msg) {
    if (!msg) return;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm);
    printf("[%s INFO] %s\n", timestamp, msg);
}

void log_error(const char *msg) {
    if (!msg) return;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm);
    printf("[%s ERROR] %s", timestamp, msg);
    if (errno != 0) {
        printf(" (errno: %d - %s)", errno, strerror(errno));
    }
    printf("\n");
}

int log_metrics_csv(const char *path,
                     float cpu_percent,
                     float mem_percent,
                     long uptime_sec,
                     float net_rx_bps,
                     float net_tx_bps) {
    if (!path) return -1;

    // Валидация пути для предотвращения path traversal атак
    if (!is_safe_log_path(path)) {
        return -1;
    }

    FILE *fp = fopen(path, "a");
    if (!fp) return -1;
    time_t now = time(NULL);
    struct tm tmv; localtime_r(&now, &tmv);
    char ts[32];

    // Безопасное форматирование времени с проверкой размера
    if (strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv) == 0) {
        strncpy(ts, "1970-01-01 00:00:00", sizeof(ts) - 1);
        ts[sizeof(ts) - 1] = '\0';
    }

    fprintf(fp, "%s,%.2f,%.2f,%ld,%.0f,%.0f\n", ts, cpu_percent, mem_percent, uptime_sec, net_rx_bps, net_tx_bps);
    fclose(fp);
    return 0;
}
