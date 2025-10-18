// disk.c
#include "disk.h"
#include <sys/statvfs.h>
#include <stdio.h>

float get_disk_usage(const char *path) {
    struct statvfs buf;
    if (statvfs(path, &buf) != 0) return 0.0f;

    unsigned long total = buf.f_blocks * buf.f_frsize;
    unsigned long free = buf.f_bfree * buf.f_frsize;

    return (float)(total - free) / total * 100.0f;
}

const char *get_disk_info() {
    static char buf[128];
    float root_usage = get_disk_usage("/");
    // Try to fetch iostat one-sample read/write KB/s (fallback, best-effort)
    float rkb = -1.0f, wkb = -1.0f;
    FILE *fp = popen("iostat -d 1 1 2>/dev/null | tail -n 1 | awk '{print $3, $4}'", "r");
    if (fp) {
        if (fscanf(fp, "%f %f", &rkb, &wkb) != 2) { rkb = -1.0f; wkb = -1.0f; }
        pclose(fp);
    }
    if (rkb >= 0.0f && wkb >= 0.0f) snprintf(buf, sizeof(buf), "/ usage: %.2f%%  R: %.1f KB/s  W: %.1f KB/s", root_usage, rkb, wkb);
    else snprintf(buf, sizeof(buf), "/ usage: %.2f%%", root_usage);
    return buf;
}
