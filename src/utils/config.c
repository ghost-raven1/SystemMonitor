// config.c
#include "utils/config.h"
#include "utils/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <errno.h>

// Функция безопасной валидации пути конфигурационного файла
static int is_safe_config_path(const char *path) {
    if (!path || strlen(path) == 0 || strlen(path) >= PATH_MAX) {
        return 0;
    }

    // Проверяем на опасные последовательности
    if (strstr(path, "..") != NULL) {
        return 0;
    }

    // Проверяем что путь начинается с разрешенного префикса
    const char *safe_prefixes[] = {"./", "src/", "config/", NULL};
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

    // Проверяем что это действительно файл с расширением .ini
    const char *ext = strrchr(path, '.');
    if (!ext || strcmp(ext, ".ini") != 0) {
        return 0;
    }

    return 1;
}

// Простейший формат KEY=VALUE, пропускаем строки с '#'
static int parse_line(const char *line, char *key, size_t ksz, char *val, size_t vsz) {
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '#' || *p == '\n' || *p == '\r') return 0;
    const char *eq = strchr(p, '=');
    if (!eq) return 0;
    size_t klen = (size_t)(eq - p);
    if (klen == 0 || klen >= ksz) return 0;
    memcpy(key, p, klen); key[klen] = '\0';
    const char *v = eq + 1;
    while (*v == ' ' || *v == '\t') v++;
    size_t vlen = strcspn(v, "\r\n");
    if (vlen >= vsz) vlen = vsz - 1;
    // Проверяем, что значение не пустое
    if (vlen == 0 || (vlen == 1 && (*v == '\r' || *v == '\n'))) {
        return 0;
    }
    memcpy(val, v, vlen); val[vlen] = '\0';
    return 1;
}

int load_config(const char *path) {
    if (!path) {
        log_error("Config path is NULL");
        return -1;
    }

    // Валидация пути для предотвращения path traversal атак
    if (!is_safe_config_path(path)) {
        log_error("Unsafe config path provided");
        return -1;
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        log_error("Failed to open config file");
        return -1;
    }

    char line[512];
    char key[128], val[384];
    int valid_lines = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!parse_line(line, key, sizeof(key), val, sizeof(val))) {
            continue;
        }

        // Валидация ключа и значения
        if (strlen(key) == 0 || strlen(val) == 0) {
            log_error("Invalid config line format: empty key or value");
            continue;
        }

        // Очищаем errno перед установкой переменной окружения
        errno = 0;

        // Применяем конфиг как переменные окружения процесса
        // Проверяем, существует ли переменная уже
        if (getenv(key) == NULL) {
            // Переменная не существует, устанавливаем её
            if (setenv(key, val, 1) != 0) {
                log_error("Failed to set environment variable");
            } else {
                valid_lines++;
            }
        } else {
            // Переменная уже существует, пропускаем
            valid_lines++;
        }
    }

    fclose(fp);

    if (valid_lines == 0) {
        log_error("No valid configuration lines found");
        return -1;
    }

    return 0;
}

int save_config(const char *path) {
    if (!path) return -1;

    // Валидация пути для предотвращения path traversal атак
    if (!is_safe_config_path(path)) {
        log_error("Unsafe config path provided for saving");
        return -1;
    }

    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    // Сохраним подмножество известных переменных
    const char *vars[] = {"SYSMON_SAFE", "SYSMON_NO_BAT", "SYSMON_NO_GPU", "SYSMON_NO_USB", "SYSMON_ANOMALY_SIGMA", NULL};
    for (int i = 0; vars[i]; i++) {
        const char *v = getenv(vars[i]);
        if (v) fprintf(fp, "%s=%s\n", vars[i], v);
    }
    fclose(fp);
    return 0;
}
