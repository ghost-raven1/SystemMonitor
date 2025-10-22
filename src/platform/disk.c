// disk.c
#include "platform/disk.h"
#include <sys/statvfs.h>
#include <stdio.h>

float get_disk_usage(const char *path) {
    if (!path) return 0.0f;

    struct statvfs buf;
    if (statvfs(path, &buf) != 0) return 0.0f;

    unsigned long long total = (unsigned long long)buf.f_blocks * buf.f_frsize;
    unsigned long long free = (unsigned long long)buf.f_bfree * buf.f_frsize;

    if (total == 0) return 0.0f;

    return (float)(total - free) / total * 100.0f;
}

const char *get_disk_info() {
    static char buf[128];
    float root_usage = get_disk_usage("/");

    // Безопасная реализация без внешних команд - показываем только использование диска
    // Убираем вызов iostat для предотвращения command injection атак

    snprintf(buf, sizeof(buf), "/ usage: %.2f%%", root_usage);
    return buf;
}
