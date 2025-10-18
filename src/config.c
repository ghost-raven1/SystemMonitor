// config.c
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    memcpy(val, v, vlen); val[vlen] = '\0';
    return 1;
}

int load_config(const char *path) {
    if (!path) return -1;
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    char line[512];
    char key[128], val[384];
    while (fgets(line, sizeof(line), fp)) {
        if (!parse_line(line, key, sizeof(key), val, sizeof(val))) continue;
        // Для простоты применяем конфиг как переменные окружения процесса
        // Перезаписываем существующие значения
        setenv(key, val, 1);
    }
    fclose(fp);
    return 0;
}

int save_config(const char *path) {
    if (!path) return -1;
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
