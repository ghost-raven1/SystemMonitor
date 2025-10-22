// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include "ui/ui.h"
#include "utils/config.h"
#include "modules/prometheus.h"
#include "utils/logging.h"
#include "utils/error_handler.h"
#include "platform/system_info.h"
#include "platform/disk.h"
#include "platform/network.h"
#include "core/app_context.h"

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
        if (is_safe_config_path(cfg)) {
            load_config(cfg);
        } else {
            log_error("Unsafe config path from environment variable");
        }
    } else {
        // Используем абсолютные пути для daemon режима
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            char src_config_path[PATH_MAX];
            char config_path[PATH_MAX];

            snprintf(src_config_path, sizeof(src_config_path), "%s/src/config.ini", cwd);
            snprintf(config_path, sizeof(config_path), "%s/config/config.ini", cwd);

            if (load_config(src_config_path) != 0) {
                if (load_config(config_path) != 0) {
                    log_error("No safe config file found");
                }
            }
        } else {
            log_error("Failed to get current working directory");
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
        // Безопасное форматирование uptime с bounds checking
        if (snprintf(up_str, sizeof(up_str), "%lds", uptime) >= (int)sizeof(up_str)) {
            strncpy(up_str, "TOO_BIG", sizeof(up_str) - 1);
            up_str[sizeof(up_str) - 1] = '\0';
        }

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
    // Настройка локали для корректного отображения UTF-8 и русских символов
    setlocale(LC_ALL, "ru_RU.UTF-8");
    setlocale(LC_CTYPE, "ru_RU.UTF-8");

    // Fallback на системную локаль если русская недоступна
    if (!setlocale(LC_ALL, "C.UTF-8")) {
        setlocale(LC_ALL, "");
    }

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
            // Очищаем ресурсы системы обработки ошибок
            error_handler_cleanup();
        
            return 0;
        }
    }

    // Инициализируем систему обработки ошибок
    error_config_t error_config = {
        .enable_graceful_degradation = true,
        .enable_auto_recovery = true,
        .max_recovery_attempts = 3,
        .recovery_cooldown_sec = 30,
        .log_file_path = "error.log",
        .custom_handler = NULL
    };
    error_handler_init(&error_config);

    // Загружаем конфигурацию с валидацией пути
    const char *cfg = getenv("SYSMON_CONFIG");
    if (cfg) {
        if (is_safe_config_path(cfg)) {
            if (load_config(cfg) != 0) {
                LOG_WARNING(ERR_CONFIG_LOAD, "Failed to load specified config file", cfg);
            }
        } else {
            LOG_WARNING(ERR_CONFIG_LOAD, "Unsafe config path provided", cfg);
        }
    } else {
        if (load_config("src/config.ini") != 0) {
            if (load_config("config.ini") != 0) {
                LOG_WARNING(ERR_CONFIG_LOAD, "No safe config file found, using defaults", "config.ini");
            }
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
        if (app_context_init(NULL) != 0) {
            LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать контекст приложения", "main");
            return -1;
        }
        run_ui();
        app_context_cleanup();
    }

    return 0;
}
