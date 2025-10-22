/**
 * @file error_handler.h
 * @brief Централизованная система обработки ошибок для SystemMonitor
 *
 * Поддерживает классификацию ошибок, graceful degradation,
 * механизмы восстановления и централизованное логирование.
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdbool.h>
#include <time.h>

// Коды ошибок для классификации
typedef enum {
    // Системные ошибки (критические)
    ERR_SYSTEM_MEMORY = 1000,
    ERR_SYSTEM_IO = 1001,
    ERR_SYSTEM_PERMISSION = 1002,
    ERR_SYSTEM_TIMEOUT = 1003,

    // Ошибки модулей мониторинга
    ERR_BATTERY_INIT = 2000,
    ERR_BATTERY_READ = 2001,
    ERR_CPU_READ = 2002,
    ERR_MEMORY_READ = 2003,
    ERR_PROCESS_READ = 2004,
    ERR_NETWORK_READ = 2005,
    ERR_DISK_READ = 2006,
    ERR_GPU_READ = 2007,

    // Ошибки пользовательского интерфейса
    ERR_UI_INIT = 3000,
    ERR_UI_RENDER = 3001,
    ERR_UI_INPUT = 3002,

    // Ошибки конфигурации
    ERR_CONFIG_LOAD = 4000,
    ERR_CONFIG_PARSE = 4001,
    ERR_CONFIG_INVALID = 4002,

    // Ошибки сети
    ERR_NETWORK_INIT = 5000,
    ERR_NETWORK_REQUEST = 5001,
    ERR_NETWORK_TIMEOUT = 5002,

    // Ошибки внешних зависимостей
    ERR_EXTERNAL_LIB = 6000,
    ERR_MISSING_DEPENDENCY = 6001,

    // Логические ошибки приложения
    ERR_INVALID_STATE = 7000,
    ERR_INVALID_PARAMETER = 7001,
    ERR_RESOURCE_EXHAUSTED = 7002
} error_code_t;

// Уровни серьезности ошибок
typedef enum {
    SEVERITY_DEBUG = 0,
    SEVERITY_INFO = 1,
    SEVERITY_WARNING = 2,
    SEVERITY_ERROR = 3,
    SEVERITY_CRITICAL = 4,
    SEVERITY_FATAL = 5
} error_severity_t;

// Структура для описания ошибки
typedef struct {
    error_code_t code;
    error_severity_t severity;
    char message[256];
    char context[128];
    char module[32];
    time_t timestamp;
    int recoverable;
    void *additional_data;
} app_error_t;

// Структура для отслеживания состояния модулей
typedef struct {
    char name[32];
    bool enabled;
    bool failed;
    int failure_count;
    time_t last_failure;
    time_t recovery_time;
} module_status_t;

// Коллбэк функция для обработки ошибок
typedef void (*error_callback_t)(const app_error_t *error);

// Структура для конфигурации обработчика ошибок
typedef struct {
    bool enable_graceful_degradation;
    bool enable_auto_recovery;
    int max_recovery_attempts;
    int recovery_cooldown_sec;
    char log_file_path[256];
    error_callback_t custom_handler;
} error_config_t;

// Глобальные функции управления ошибками
void error_handler_init(const error_config_t *config);
void error_handler_cleanup(void);

// Создание и регистрация ошибок
app_error_t create_error(error_code_t code, error_severity_t severity,
                        const char *message, const char *context, const char *module);
void register_error(const app_error_t *error);

// Проверка состояния модулей
bool is_module_enabled(const char *module_name);
bool is_module_failed(const char *module_name);
int get_module_failure_count(const char *module_name);

// Graceful degradation
void disable_module(const char *module_name, const char *reason);
void enable_module(const char *module_name);
bool should_attempt_recovery(const char *module_name);

// Восстановление после ошибок
void attempt_module_recovery(const char *module_name);
void reset_module_failures(const char *module_name);

// Удобные макросы для создания ошибок
#define LOG_ERROR(code, msg, context) \
    do { \
        app_error_t error = create_error(code, SEVERITY_ERROR, msg, context, __func__); \
        register_error(&error); \
    } while(0)

#define LOG_WARNING(code, msg, context) \
    do { \
        app_error_t error = create_error(code, SEVERITY_WARNING, msg, context, __func__); \
        register_error(&error); \
    } while(0)

#define LOG_CRITICAL(code, msg, context) \
    do { \
        app_error_t error = create_error(code, SEVERITY_CRITICAL, msg, context, __func__); \
        register_error(&error); \
    } while(0)

#define LOG_INFO(msg, context) \
    do { \
        app_error_t error = create_error(0, SEVERITY_INFO, msg, context, __func__); \
        register_error(&error); \
    } while(0)

#define LOG_DEBUG(code, msg, context) \
    do { \
        app_error_t error = create_error(code, SEVERITY_DEBUG, msg, context, __func__); \
        register_error(&error); \
    } while(0)

// Макросы для graceful degradation
#define CHECK_MODULE_ENABLED(module) \
    if (!is_module_enabled(module)) return -1;

#define HANDLE_MODULE_FAILURE(module, reason) \
    do { \
        LOG_ERROR(ERR_INVALID_STATE, "Module failure", reason); \
        disable_module(module, reason); \
    } while(0)

// Макросы для безопасного выполнения операций
#define SAFE_EXECUTE(operation, error_action) \
    do { \
        if (operation != 0) { \
            error_action; \
        } \
    } while(0)

#endif // ERROR_HANDLER_H