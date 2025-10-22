/**
 * @file developer_tools.h
 * @brief Интерфейс модуля инструментов разработчика
 *
 * Предоставляет инструменты для отладки, профилирования,
 * анализа производительности и диагностики приложения.
 */

#ifndef DEVELOPER_TOOLS_H
#define DEVELOPER_TOOLS_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include "../core/developer.h"

// Режимы профилирования
typedef enum {
    PROFILING_DISABLED = 0,
    PROFILING_CPU = 1,
    PROFILING_MEMORY = 2,
    PROFILING_IO = 4,
    PROFILING_NETWORK = 8,
    PROFILING_ALL = 15
} profiling_mode_t;

// Используем определение из core/developer.h

// Основная структура инструментов разработчика
typedef struct {
    bool initialized;
    bool monitoring_active;
    bool ide_monitoring_active;
    dev_environment_t environment;
    char workspace_path[512];
} developer_tools_t;

// Структура для точки профилирования
typedef struct {
    char name[64];             // Название точки профилирования
    time_t timestamp;          // Временная метка
    unsigned long cpu_time_us; // Время CPU в микросекундах
    unsigned long memory_kb;   // Использование памяти в KB
    int thread_id;            // ID потока
    char category[32];        // Категория (render, compute, io и т.д.)
} profiling_point_t;

// Структура для статистики функции
typedef struct {
    char function_name[128];   // Имя функции
    unsigned long call_count;  // Количество вызовов
    unsigned long total_time_us; // Общее время выполнения в микросекундах
    unsigned long min_time_us; // Минимальное время выполнения
    unsigned long max_time_us; // Максимальное время выполнения
    unsigned long memory_delta_kb; // Изменение памяти за время выполнения
} function_stats_t;

// Структура для отчета памяти
typedef struct {
    unsigned long heap_allocated;  // Выделено в heap
    unsigned long heap_used;       // Используется в heap
    unsigned long stack_size;      // Размер стека
    int allocation_count;          // Количество активных выделений
    int leak_suspicions;          // Подозрения на утечки памяти
} memory_report_t;

// Структура для лога событий
typedef struct {
    char event_type[32];       // Тип события (debug, info, warning, error)
    char message[256];         // Сообщение события
    char module[32];          // Модуль, генерировавший событие
    time_t timestamp;         // Временная метка
    int severity;             // Уровень серьезности (0-5)
} debug_event_t;

// Структура состояния модуля инструментов разработчика
typedef struct {
    bool profiling_enabled;
    bool memory_tracking_enabled;
    bool debug_logging_enabled;
    profiling_mode_t profiling_mode;
    int max_log_entries;
    char output_directory[256];
} developer_tools_state_t;

// Инициализация и деинициализация модуля
int developer_tools_init(const char *output_dir);
void developer_tools_cleanup(void);
int developer_tools_enable(void);
int developer_tools_disable(void);

// Профилирование производительности
int start_profiling(profiling_mode_t mode);
int stop_profiling(void);
int add_profiling_point(const char *name, const char *category);

// Получение статистики профилирования
int get_function_stats(function_stats_t **stats, int *count);
int get_profiling_report(char *buffer, int buffer_size);
void free_function_stats(function_stats_t *stats);

// Отслеживание памяти
int start_memory_tracking(void);
int stop_memory_tracking(void);
int get_memory_report(memory_report_t *report);
int detect_memory_leaks(void);

// Логирование событий разработки
int log_debug_event(const char *event_type, const char *message,
                   const char *module, int severity);
int get_debug_events(debug_event_t **events, int *count);
void free_debug_events(debug_event_t *events);

// Анализ кода и производительности
int analyze_function_performance(const char *function_name);
int detect_performance_bottlenecks(void);
int generate_performance_report(const char *filename);

// Коллбэки для мониторинга
typedef void (*profiling_callback_t)(const profiling_point_t *point);
typedef void (*memory_callback_t)(const memory_report_t *report);

// Регистрация коллбэков
int register_profiling_callback(profiling_callback_t callback);
int register_memory_callback(memory_callback_t callback);

// Конфигурация модуля
int set_profiling_mode(profiling_mode_t mode);
int set_memory_tracking(bool enabled);
int set_debug_logging(bool enabled, int max_entries);
int set_output_directory(const char *directory);

// Получение состояния модуля
const developer_tools_state_t *get_developer_tools_state(void);

// Функции создания и уничтожения Developer Tools
developer_tools_t *developer_tools_create(void);
int developer_tools_destroy(developer_tools_t *tools);

// Функции анализа окружения разработки
int developer_tools_get_environment(developer_tools_t *tools, dev_environment_t *environment);

// Функции анализа процессов разработки
int developer_tools_get_dev_processes(developer_tools_t *tools, dev_process_t *processes,
                                     int max_processes, int *process_count);

// Функции анализа портов разработки
int developer_tools_get_listening_ports(developer_tools_t *tools, port_info_t *ports,
                                       int max_ports, int *port_count);

// Функции мониторинга ресурсов разработки
int developer_tools_get_metrics(developer_tools_t *tools, dev_metrics_t *metrics);
int developer_tools_start_monitoring(developer_tools_t *tools);
int developer_tools_stop_monitoring(developer_tools_t *tools);
bool developer_tools_is_monitoring(const developer_tools_t *tools);

// Функции диагностики разработки
int developer_tools_get_performance_diagnostics(developer_tools_t *tools, dev_performance_t *performance);
int developer_tools_get_diagnostics(developer_tools_t *tools, char *buffer, int buffer_size);

// Функции интеграции с IDE
int developer_tools_get_ide_info(developer_tools_t *tools, ide_info_t *ide_info);
int developer_tools_start_ide_monitoring(developer_tools_t *tools);
int developer_tools_stop_ide_monitoring(developer_tools_t *tools);
bool developer_tools_is_ide_monitoring(const developer_tools_t *tools);

// Макросы для удобного профилирования
#define PROFILE_FUNCTION(category) \
    static const char __func_name[] = __func__; \
    add_profiling_point(__func_name, category)

#define PROFILE_SCOPE(name, category) \
    struct { time_t _start; } __prof_scope = { .start = time(NULL) }; \
    add_profiling_point(name, category)

#define LOG_DEV_EVENT(type, msg, module, severity) \
    log_debug_event(type, msg, module, severity)

// Условная компиляция для отладочных сборок
#ifdef DEBUG_BUILD
#define DEBUG_LOG(msg) LOG_DEV_EVENT("debug", msg, __func__, 1)
#else
#define DEBUG_LOG(msg) ((void)0)
#endif

#endif // DEVELOPER_TOOLS_H