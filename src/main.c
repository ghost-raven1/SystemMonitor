// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ui.h"
#include "config.h"
#include "prometheus.h"
#include "logging.h"
#include "system_info.h"
#include "disk.h"
#include "network.h"

static volatile sig_atomic_t daemon_running = 1;

static void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        daemon_running = 0;
    }
}

static void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Родительский процесс завершается
    }

    // Создаем новую сессию
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Изменяем рабочий каталог
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    // Закрываем стандартные файловые дескрипторы
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Перенаправляем в /dev/null
    open("/dev/null", O_RDONLY);  // stdin
    open("/dev/null", O_WRONLY);  // stdout
    open("/dev/null", O_WRONLY);  // stderr
}

static void print_non_interactive_metrics(void) {
    const char *cfg = getenv("SYSMON_CONFIG");
    if (cfg) {
        load_config(cfg);
    } else {
        if (load_config("src/config.ini") != 0) {
            load_config("config.ini");
        }
    }

    log_info("Starting SysMon in non-interactive mode");

    // Основной цикл мониторинга
    while (daemon_running) {
        float cpu = get_cpu_usage();
        float mem = get_memory_usage();
        long uptime = get_uptime_seconds();
        const char *disk_info = get_disk_info();
        const char *net_info = get_network_info();

        char up_str[64];
        snprintf(up_str, sizeof(up_str), "%lds", uptime);

        printf("CPU: %.1f%%  MEM: %.1f%%  UPTIME: %s  DISK: %s  NET: %s\n",
               cpu >= 0 ? cpu : 0.0f,
               mem >= 0 ? mem : 0.0f,
               up_str,
               disk_info ? disk_info : "N/A",
               net_info ? net_info : "N/A");

        // Экспорт в Prometheus если включен
        const char *prometheus_enabled = getenv("SYSMON_PROMETHEUS");
        if (prometheus_enabled && prometheus_enabled[0] == '1') {
            const char *metrics = prometheus_get_metrics();
            printf("\n--- PROMETHEUS METRICS ---\n%s\n", metrics);
        }

        sleep(5); // Обновление каждые 5 секунд
    }

    log_info("SysMon daemon stopped");
}

int main(int argc, char *argv[]) {
    // Проверяем аргументы командной строки
    int daemon_mode = 0;
    int prometheus_mode = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "-d") == 0) {
            daemon_mode = 1;
        } else if (strcmp(argv[i], "--prometheus") == 0 || strcmp(argv[i], "-p") == 0) {
            prometheus_mode = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("SysMon v2.0 - System Monitor\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -d, --daemon       Run in daemon mode\n");
            printf("  -p, --prometheus   Enable Prometheus metrics export\n");
            printf("  -h, --help         Show this help\n");
            printf("\nEnvironment variables:\n");
            printf("  SYSMON_CONFIG      Path to config file\n");
            printf("  SYSMON_SAFE        Safe mode (1=enabled)\n");
            printf("  SYSMON_DEBUG       Debug mode (1=enabled)\n");
            return 0;
        }
    }

    // Загружаем конфигурацию
    const char *cfg = getenv("SYSMON_CONFIG");
    if (cfg) {
        load_config(cfg);
    } else {
        if (load_config("src/config.ini") != 0) {
            load_config("config.ini");
        }
    }

    // Устанавливаем обработчики сигналов
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    if (daemon_mode) {
        // Daemon режим
        log_info("Starting SysMon daemon");
        daemonize();

        if (prometheus_mode) {
            // Prometheus сервер режим
            prometheus_start_server(9090);
        } else {
            // Простой daemon режим
            print_non_interactive_metrics();
        }
    } else if (prometheus_mode) {
        // Только Prometheus сервер (не демон)
        prometheus_start_server(9090);
    } else {
        // Интерактивный режим по умолчанию
        run_ui();
    }

    return 0;
}
