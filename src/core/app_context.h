/**
 * @file app_context.h
 * @brief Контекст приложения для управления модулями SystemMonitor
 *
 * Централизованная структура для управления состоянием приложения,
 * координации модулей и предоставления глобального доступа к ресурсам.
 */

#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "modules/system_monitor.h"
#include "modules/developer_tools.h"
#include "modules/diagnostics.h"
#include "utils/error_handler.h"

// Статус приложения
typedef enum {
    APP_STATUS_STOPPED = 0,    // Остановлено
    APP_STATUS_STARTING = 1,   // Запускается
    APP_STATUS_RUNNING = 2,    // Работает
    APP_STATUS_STOPPING = 3,   // Останавливается
    APP_STATUS_ERROR = 4,      // Ошибка
    APP_STATUS_PAUSED = 5      // Приостановлено
} app_status_t;

// Режимы работы приложения
typedef enum {
    APP_MODE_NORMAL = 0,       // Нормальный режим
    APP_MODE_DEMO = 1,         // Демо режим
    APP_MODE_DEBUG = 2,        // Режим отладки
    APP_MODE_DIAGNOSTICS = 3,  // Диагностический режим
    APP_MODE_MAINTENANCE = 4   // Режим обслуживания
} app_mode_t;

// Структура для конфигурации приложения
typedef struct {
    char config_file_path[256];    // Путь к файлу конфигурации
    char data_directory[256];      // Директория для данных
    char log_directory[256];       // Директория для логов
    char temp_directory[256];      // Временная директория
    int max_log_files;             // Максимум файлов логов
    int max_log_file_size_mb;      // Максимальный размер файла лога
    bool enable_networking;        // Включить сетевые функции
    bool enable_notifications;     // Включить уведомления
    int update_interval_ms;         // Интервал обновления в мс
} app_config_t;

// Структура для глобального состояния приложения
typedef struct {
    app_status_t status;           // Текущий статус
    app_mode_t mode;              // Текущий режим
    char version[32];             // Версия приложения
    char build_date[32];          // Дата сборки
    pid_t main_pid;               // PID главного процесса
    time_t start_time;            // Время запуска
    time_t last_update;           // Последнее обновление
    pthread_t main_thread;        // Главный поток
    bool shutdown_requested;      // Запрос на завершение работы
    int exit_code;                // Код завершения
} app_global_state_t;

// Структура для управления модулями
typedef struct {
    bool system_monitor_initialized;
    bool developer_tools_initialized;
    bool diagnostics_initialized;
    bool error_handler_initialized;
    bool ui_initialized;
    int active_modules_count;     // Количество активных модулей
    int failed_modules_count;     // Количество модулей с ошибками
    time_t last_module_check;     // Последняя проверка модулей
} module_manager_t;

// Структура для статистики производительности
typedef struct {
    unsigned long total_memory_usage_kb;  // Общее использование памяти
    float average_cpu_usage;       // Среднее использование CPU
    int active_threads_count;      // Количество активных потоков
    unsigned long total_allocations; // Общее количество выделений памяти
    unsigned long total_deallocations; // Общее количество освобождений
    time_t measurement_start;      // Начало измерения статистики
} performance_stats_t;

// Структура для обработчиков событий
typedef struct {
    void (*on_status_changed)(app_status_t old_status, app_status_t new_status);
    void (*on_mode_changed)(app_mode_t old_mode, app_mode_t new_mode);
    void (*on_module_initialized)(const char *module_name);
    void (*on_module_failed)(const char *module_name, const char *error);
    void (*on_shutdown)(int exit_code);
} event_handlers_t;

// Главная структура контекста приложения
typedef struct {
    app_config_t config;           // Конфигурация приложения
    app_global_state_t global_state; // Глобальное состояние
    module_manager_t modules;      // Управление модулями
    performance_stats_t performance; // Статистика производительности
    event_handlers_t events;       // Обработчики событий
    pthread_mutex_t mutex;         // Мьютекс для синхронизации
    pthread_cond_t status_changed; // Условная переменная для изменения статуса
} app_context_t;

// Глобальные функции инициализации
int app_context_init(const char *config_file);
int app_context_init_with_config(const app_config_t *config);
void app_context_cleanup(void);

// Управление жизненным циклом приложения
int app_context_start(void);
int app_context_stop(void);
int app_context_pause(void);
int app_context_resume(void);
int app_context_restart(void);

// Получение глобального контекста
app_context_t *get_app_context(void);
const app_context_t *get_app_context_readonly(void);

// Управление статусом и режимами
int set_app_status(app_status_t status);
int set_app_mode(app_mode_t mode);
app_status_t get_app_status(void);
app_mode_t get_app_mode(void);

// Управление модулями через контекст
int initialize_module(const char *module_name);
int shutdown_module(const char *module_name);
bool is_module_initialized(const char *module_name);
int get_initialized_modules(char *buffer, int buffer_size);

// Обновление состояния приложения
int update_app_context(void);
int refresh_performance_stats(void);

// Регистрация обработчиков событий
int register_status_change_handler(void (*handler)(app_status_t, app_status_t));
int register_mode_change_handler(void (*handler)(app_mode_t, app_mode_t));
int register_module_handler(void (*init_handler)(const char *),
                           void (*fail_handler)(const char *, const char *));
int register_shutdown_handler(void (*handler)(int));

// Конфигурация приложения
int load_app_config(const char *config_file);
int save_app_config(const char *config_file);
int update_app_config(const app_config_t *new_config);
const app_config_t *get_app_config(void);

// Утилиты для работы с контекстом
int execute_with_context_lock(int (*function)(void *), void *arg);
bool is_app_context_valid(void);
const char *get_app_status_string(app_status_t status);
const char *get_app_mode_string(app_mode_t mode);

// Макросы для безопасной работы с контекстом
#define WITH_CONTEXT_LOCK(code) \
    do { \
        pthread_mutex_lock(&get_app_context()->mutex); \
        code; \
        pthread_mutex_unlock(&get_app_context()->mutex); \
    } while(0)

#define SAFE_CONTEXT_CALL(func, ...) \
    (is_app_context_valid() ? func(__VA_ARGS__) : -1)

// Проверка состояния приложения
#define IS_APP_RUNNING() (get_app_status() == APP_STATUS_RUNNING)
#define IS_APP_STOPPED() (get_app_status() == APP_STATUS_STOPPED)
#define IS_APP_ERROR() (get_app_status() == APP_STATUS_ERROR)

// Проверка активности главного цикла приложения
int app_context_is_running(void);

// Автоматическая регистрация модулей
#define REGISTER_MODULE(module_name) \
    do { \
        if (initialize_module(#module_name) == 0) { \
            LOG_INFO("Module " #module_name " initialized successfully", "app_context"); \
        } else { \
            LOG_ERROR(ERR_EXTERNAL_LIB, "Failed to initialize module " #module_name, "app_context"); \
        } \
    } while(0)

// Глобальные переменные для быстрого доступа
extern app_context_t *g_app_context;
extern bool g_context_initialized;

// Функции для работы с глобальными переменными
#define GET_GLOBAL_CONTEXT() (g_context_initialized ? g_app_context : NULL)

#endif // APP_CONTEXT_H