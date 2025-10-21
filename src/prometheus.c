// prometheus.c - HTTP сервер для экспорта метрик в формате Prometheus
#include "prometheus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "system_info.h"
#include "processes.h"
#include "network.h"
#include "disk.h"
#include "battery.h"
#include "gpu.h"
#include "logging.h"

// Структура для хранения метрик
typedef struct {
    char name[64];
    float value;
    char type[16]; // "gauge", "counter", "histogram"
    char help[128];
} prometheus_metric_t;

// Глобальный буфер метрик
static char metrics_buffer[8192];
static time_t last_metrics_update = 0;

static void add_metric(const char *name, float value, const char *type, const char *help) {
    static int first = 1;
    if (first) {
        snprintf(metrics_buffer, sizeof(metrics_buffer),
                "# HELP %s %s\n# TYPE %s %s\n%s %.2f\n",
                name, help, name, type, name, value);
        first = 0;
    } else {
        char temp[1024];
        snprintf(temp, sizeof(temp), "%s %.2f\n", name, value);
        strncat(metrics_buffer, temp, sizeof(metrics_buffer) - strlen(metrics_buffer) - 1);
    }
}

static void collect_system_metrics(void) {
    // CPU метрики
    float cpu = get_cpu_usage();
    if (cpu >= 0) {
        add_metric("sysmon_cpu_usage_percent", cpu, "gauge",
                  "Current CPU usage percentage across all cores");
    }

    // Memory метрики
    float mem = get_memory_usage();
    if (mem >= 0) {
        add_metric("sysmon_memory_usage_percent", mem, "gauge",
                  "Current memory usage percentage");
    }

    // Memory breakdown
    memory_breakdown_t mb;
    if (get_memory_breakdown(&mb) == 0) {
        add_metric("sysmon_memory_free_bytes", (float)mb.free_bytes, "gauge",
                  "Free memory in bytes");
        add_metric("sysmon_memory_active_bytes", (float)mb.active_bytes, "gauge",
                  "Active memory in bytes");
        add_metric("sysmon_memory_wired_bytes", (float)mb.wired_bytes, "gauge",
                  "Wired memory in bytes");
    }

    // Uptime
    long uptime = get_uptime_seconds();
    if (uptime >= 0) {
        add_metric("sysmon_uptime_seconds", (float)uptime, "counter",
                  "System uptime in seconds");
    }

    // CPU frequencies
    long long curr_hz, max_hz;
    if (get_cpu_frequencies(&curr_hz, &max_hz) == 0) {
        if (curr_hz > 0) {
            add_metric("sysmon_cpu_frequency_hz", (float)curr_hz, "gauge",
                      "Current CPU frequency in Hz");
        }
        if (max_hz > 0) {
            add_metric("sysmon_cpu_frequency_max_hz", (float)max_hz, "gauge",
                      "Maximum CPU frequency in Hz");
        }
    }

    // Network метрики
    float rx_rate = get_network_rx();
    float tx_rate = get_network_tx();
    add_metric("sysmon_network_rx_bytes_per_second", rx_rate, "gauge",
              "Network receive rate in bytes per second");
    add_metric("sysmon_network_tx_bytes_per_second", tx_rate, "gauge",
              "Network transmit rate in bytes per second");

    // Disk метрики
    float disk_usage = get_disk_usage("/");
    add_metric("sysmon_disk_usage_percent", disk_usage, "gauge",
              "Root filesystem usage percentage");

    // Battery метрики (macOS)
    battery_info_t battery;
    if (get_battery_info(&battery) == 0) {
        if (battery.percentage >= 0) {
            add_metric("sysmon_battery_percentage", battery.percentage, "gauge",
                      "Battery charge percentage");
        }
        if (battery.charging >= 0) {
            add_metric("sysmon_battery_charging", (float)battery.charging, "gauge",
                      "Battery charging status (1=charging, 0=discharging)");
        }
        if (battery.time_remaining_min >= 0) {
            add_metric("sysmon_battery_time_remaining_minutes", (float)battery.time_remaining_min, "gauge",
                      "Battery time remaining in minutes");
        }
    }

    // GPU метрики
    gpu_info_t gpu;
    if (get_gpu_info(&gpu) == 0) {
        if (gpu.usage >= 0) {
            add_metric("sysmon_gpu_usage_percent", gpu.usage, "gauge",
                      "GPU usage percentage");
        }
        if (gpu.temperature >= 0) {
            add_metric("sysmon_gpu_temperature_celsius", gpu.temperature, "gauge",
                      "GPU temperature in Celsius");
        }
        if (gpu.memory_used > 0) {
            add_metric("sysmon_gpu_memory_used_mb", (float)gpu.memory_used, "gauge",
                      "GPU memory used in MB");
        }
        if (gpu.memory_total > 0) {
            add_metric("sysmon_gpu_memory_total_mb", (float)gpu.memory_total, "gauge",
                      "GPU memory total in MB");
        }
    }

    // Process count
    process_info_t procs[256];
    size_t proc_count = get_process_list(procs, 256);
    add_metric("sysmon_process_count", (float)proc_count, "gauge",
              "Total number of processes");

    // Process CPU/Memory stats
    float total_proc_cpu = 0, total_proc_mem = 0;
    for (size_t i = 0; i < proc_count && i < 10; i++) {
        total_proc_cpu += procs[i].cpu_usage;
        total_proc_mem += procs[i].mem_usage;
    }
    if (proc_count > 0) {
        add_metric("sysmon_top_processes_cpu_percent", total_proc_cpu, "gauge",
                  "Total CPU usage of top 10 processes");
        add_metric("sysmon_top_processes_memory_percent", total_proc_mem, "gauge",
                  "Total memory usage of top 10 processes");
    }
}

void prometheus_export_metric(const char *name, float value) {
    // Для обратной совместимости
    printf("# HELP %s Custom metric\n# TYPE %s gauge\n%s %.2f\n", name, name, name, value);
}

const char *prometheus_get_metrics(void) {
    time_t now = time(NULL);

    // Обновляем метрики не чаще чем раз в 5 секунд
    if (last_metrics_update == 0 || (now - last_metrics_update) >= 5) {
        memset(metrics_buffer, 0, sizeof(metrics_buffer));
        collect_system_metrics();
        last_metrics_update = now;
    }

    return metrics_buffer;
}

int prometheus_start_server(int port) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Starting Prometheus metrics server on port %d", port);
    log_info(buf);

    // Заглушка - в полной реализации здесь должен быть HTTP сервер
    // Пока что просто выводим метрики в stdout каждые 30 секунд

    while (1) {
        printf("\n=== PROMETHEUS METRICS ===\n");
        printf("%s\n", prometheus_get_metrics());
        printf("=== END METRICS ===\n");

        sleep(30);
    }

    return 0;
}
