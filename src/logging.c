// logging.c
#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

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
    FILE *fp = fopen(path, "a");
    if (!fp) return -1;
    time_t now = time(NULL);
    struct tm tmv; localtime_r(&now, &tmv);
    char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(fp, "%s,%.2f,%.2f,%ld,%.0f,%.0f\n", ts, cpu_percent, mem_percent, uptime_sec, net_rx_bps, net_tx_bps);
    fclose(fp);
    return 0;
}
