/**
 * @file error_handler.c
 * @brief Реализация централизованной системы обработки ошибок
 */

#include "utils/error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Глобальные переменные
static error_config_t g_config;
static module_status_t g_modules[16];
static int g_module_count = 0;
static pthread_mutex_t g_error_mutex = PTHREAD_MUTEX_INITIALIZER;

// Список известных модулей системы
static const char *KNOWN_MODULES[] = {
    "battery", "cpu", "memory", "processes", "network", "disk",
    "gpu", "ui", "config", "notifications", "prometheus", NULL
};

static const char *SEVERITY_STRINGS[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "FATAL"
};

// Внутренние функции
static void default_error_handler(const app_error_t *error);
static void log_error_to_file(const app_error_t *error);
static module_status_t *find_module(const char *module_name);
static void add_known_module(const char *module_name);

// Инициализация системы обработки ошибок
void error_handler_init(const error_config_t *config) {
    if (!config) return;

    memcpy(&g_config, config, sizeof(error_config_t));

    // Инициализируем известные модули
    for (int i = 0; KNOWN_MODULES[i] != NULL; i++) {
        add_known_module(KNOWN_MODULES[i]);
    }

    LOG_INFO("Error handler initialized", "System startup");
}

// Очистка ресурсов
void error_handler_cleanup(void) {
    pthread_mutex_lock(&g_error_mutex);

    for (int i = 0; i < g_module_count; i++) {
        if (g_modules[i].failed) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Module %s had %d failures",
                     g_modules[i].name, g_modules[i].failure_count);
            LOG_INFO(buf, "Cleanup summary");
        }
    }

    pthread_mutex_unlock(&g_error_mutex);
    app_error_t cleanup_error = create_error(
        ERR_INVALID_STATE, SEVERITY_INFO,
        "Error handler cleanup completed", "System shutdown", "error_handler");
    register_error(&cleanup_error);
}

// Создание структуры ошибки
app_error_t create_error(error_code_t code, error_severity_t severity,
                        const char *message, const char *context, const char *module) {
    app_error_t error;

    error.code = code;
    error.severity = severity;
    error.timestamp = time(NULL);
    error.recoverable = (severity < SEVERITY_CRITICAL);

    if (message) {
        strncpy(error.message, message, sizeof(error.message) - 1);
        error.message[sizeof(error.message) - 1] = '\0';
    } else {
        strcpy(error.message, "Unknown error");
    }

    if (context) {
        strncpy(error.context, context, sizeof(error.context) - 1);
        error.context[sizeof(error.context) - 1] = '\0';
    } else {
        strcpy(error.context, "No context");
    }

    if (module) {
        strncpy(error.module, module, sizeof(error.module) - 1);
        error.module[sizeof(error.module) - 1] = '\0';
    } else {
        strcpy(error.module, "unknown");
    }

    return error;
}

// Регистрация ошибки
void register_error(const app_error_t *error) {
    if (!error) return;

    pthread_mutex_lock(&g_error_mutex);

    // Логируем ошибку
    if (error->severity >= SEVERITY_INFO) {
        default_error_handler(error);
    }

    // Обрабатываем graceful degradation
    if (g_config.enable_graceful_degradation &&
        error->severity >= SEVERITY_ERROR) {
        module_status_t *module = find_module(error->module);
        if (module && !module->failed) {
            module->failed = true;
            module->failure_count++;
            module->last_failure = time(NULL);

            // Отключаем модуль если он критически важен
            if (error->severity >= SEVERITY_CRITICAL) {
                disable_module(error->module, error->message);
            }
        }
    }

    pthread_mutex_unlock(&g_error_mutex);
}

// Обработчик ошибок по умолчанию
static void default_error_handler(const app_error_t *error) {
    // Вывод в консоль
    printf("[%s] %s:%s [%s] %s",
           SEVERITY_STRINGS[error->severity],
           error->module,
           error->context,
           error->message,
           (error->severity >= SEVERITY_ERROR) ? "\n" : "");

    // Логирование в файл если настроено
    if (strlen(g_config.log_file_path) > 0) {
        log_error_to_file(error);
    }

    // Вызов пользовательского обработчика
    if (g_config.custom_handler) {
        g_config.custom_handler(error);
    }
}

// Логирование ошибки в файл
static void log_error_to_file(const app_error_t *error) {
    FILE *fp = fopen(g_config.log_file_path, "a");
    if (!fp) return;

    struct tm *tm = localtime(&error->timestamp);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(fp, "[%s] %s:%s [%s] %s\n",
            timestamp,
            SEVERITY_STRINGS[error->severity],
            error->module,
            error->context,
            error->message);

    fclose(fp);
}

// Поиск модуля в списке
static module_status_t *find_module(const char *module_name) {
    if (!module_name) return NULL;

    for (int i = 0; i < g_module_count; i++) {
        if (strcmp(g_modules[i].name, module_name) == 0) {
            return &g_modules[i];
        }
    }

    // Если модуль не найден, добавляем его
    if (g_module_count < (int)(sizeof(g_modules) / sizeof(g_modules[0]))) {
        add_known_module(module_name);
        return &g_modules[g_module_count - 1];
    }

    return NULL;
}

// Добавление известного модуля
static void add_known_module(const char *module_name) {
    if (!module_name || g_module_count >= (int)(sizeof(g_modules) / sizeof(g_modules[0]))) {
        return;
    }

    strncpy(g_modules[g_module_count].name, module_name,
            sizeof(g_modules[g_module_count].name) - 1);
    g_modules[g_module_count].name[sizeof(g_modules[g_module_count].name) - 1] = '\0';

    g_modules[g_module_count].enabled = true;
    g_modules[g_module_count].failed = false;
    g_modules[g_module_count].failure_count = 0;
    g_modules[g_module_count].last_failure = 0;
    g_modules[g_module_count].recovery_time = 0;

    g_module_count++;
}

// Проверка состояния модулей
bool is_module_enabled(const char *module_name) {
    module_status_t *module = find_module(module_name);
    return module ? module->enabled : false;
}

bool is_module_failed(const char *module_name) {
    module_status_t *module = find_module(module_name);
    return module ? module->failed : false;
}

int get_module_failure_count(const char *module_name) {
    module_status_t *module = find_module(module_name);
    return module ? module->failure_count : 0;
}

// Graceful degradation
void disable_module(const char *module_name, const char *reason) {
    if (!module_name) return;

    pthread_mutex_lock(&g_error_mutex);

    module_status_t *module = find_module(module_name);
    if (module) {
        module->enabled = false;
        module->failed = true;
        module->recovery_time = time(NULL) + g_config.recovery_cooldown_sec;

        app_error_t disable_error = create_error(
            ERR_INVALID_STATE, SEVERITY_WARNING,
            "Module disabled", reason ? reason : "No reason provided",
            module_name);
        register_error(&disable_error);
    }

    pthread_mutex_unlock(&g_error_mutex);
}

void enable_module(const char *module_name) {
    if (!module_name) return;

    pthread_mutex_lock(&g_error_mutex);

    module_status_t *module = find_module(module_name);
    if (module) {
        module->enabled = true;
        module->failed = false;
        module->recovery_time = 0;

        app_error_t enable_error = create_error(
            ERR_INVALID_STATE, SEVERITY_INFO,
            "Module re-enabled", "Module recovered", module_name);
        register_error(&enable_error);
    }

    pthread_mutex_unlock(&g_error_mutex);
}

bool should_attempt_recovery(const char *module_name) {
    if (!g_config.enable_auto_recovery) return false;

    module_status_t *module = find_module(module_name);
    if (!module || !module->failed) return false;

    // Проверяем cooldown период
    if (module->recovery_time > time(NULL)) return false;

    // Проверяем максимальное количество попыток восстановления
    if (module->failure_count >= g_config.max_recovery_attempts) return false;

    return true;
}

// Восстановление после ошибок
void attempt_module_recovery(const char *module_name) {
    if (!module_name || !should_attempt_recovery(module_name)) return;

    pthread_mutex_lock(&g_error_mutex);

    module_status_t *module = find_module(module_name);
    if (module) {
        module->recovery_time = time(NULL) + g_config.recovery_cooldown_sec;

        app_error_t recovery_error = create_error(
            ERR_INVALID_STATE, SEVERITY_INFO,
            "Attempting module recovery", "Starting recovery attempt",
            module_name);
        register_error(&recovery_error);

        // Здесь можно добавить специфичную логику восстановления
        // для каждого модуля в будущем
        enable_module(module_name);
    }

    pthread_mutex_unlock(&g_error_mutex);
}

void reset_module_failures(const char *module_name) {
    if (!module_name) return;

    pthread_mutex_lock(&g_error_mutex);

    module_status_t *module = find_module(module_name);
    if (module) {
        module->failure_count = 0;
        module->last_failure = 0;
        module->recovery_time = 0;

        app_error_t reset_error = create_error(
            ERR_INVALID_STATE, SEVERITY_INFO,
            "Module failures reset", "Manual reset", module_name);
        register_error(&reset_error);
    }

    pthread_mutex_unlock(&g_error_mutex);
}