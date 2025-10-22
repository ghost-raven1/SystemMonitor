/**
 * @file system_monitor.h
 * @brief Интерфейс модуля мониторинга системных ресурсов
 *
 * Предоставляет функции для сбора информации о CPU, памяти,
 * процессах и других системных ресурсах с унифицированным интерфейсом.
 */

#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdbool.h>
#include <time.h>

// Структура для информации о CPU
typedef struct {
    float usage_percent;        // Процент использования CPU (0-100)
    int core_count;            // Количество ядер
    float temperature;         // Температура в градусах Цельсия (-1 если недоступно)
    float frequency;           // Текущая частота в MHz (-1 если недоступно)
} cpu_info_t;

// Структура для информации о памяти
typedef struct {
    unsigned long total_mb;    // Общий объем памяти в MB
    unsigned long used_mb;     // Используемая память в MB
    unsigned long free_mb;     // Свободная память в MB
    float usage_percent;       // Процент использования памяти
    unsigned long swap_total_mb; // Общий объем swap в MB
    unsigned long swap_used_mb;  // Используемый swap в MB
} memory_info_t;

// Структура для информации о системе
typedef struct {
    char platform[32];         // Название платформы (Linux, macOS, Windows)
    char hostname[64];         // Имя хоста
    char os_version[64];       // Версия ОС
    int uptime_seconds;        // Время работы системы в секундах
    time_t boot_time;          // Время загрузки системы
} system_info_t;

// Структура для процесса (новая версия с дополнительными полями)
typedef struct {
    int pid;                   // ID процесса
    int ppid;                  // ID родительский процесс
    char name[256];           // Имя процесса
    char user[32];            // Пользователь
    float cpu_percent;        // Использование CPU в процентах
    unsigned long memory_kb;  // Использование памяти в KB
    int thread_count;         // Количество потоков
    time_t start_time;        // Время запуска
} system_process_info_t;

// Структура для состояния модуля мониторинга
typedef struct {
    bool cpu_monitor_enabled;
    bool memory_monitor_enabled;
    bool process_monitor_enabled;
    bool system_monitor_enabled;
    int update_interval_ms;    // Интервал обновления в миллисекундах
    time_t last_update;       // Время последнего обновления
} system_monitor_state_t;

// Структура для статистики модуля мониторинга
typedef struct {
    int cycles;                // Количество циклов обновления
    int history_index;         // Текущий индекс истории
    time_t last_update;        // Время последнего обновления
    int update_interval_ms;    // Интервал обновления в миллисекундах
    float avg_cpu_usage;       // Среднее использование CPU
    float avg_memory_usage;    // Среднее использование памяти
    int valid_samples;         // Количество валидных сэмплов
} system_monitor_stats_t;

// Инициализация и деинициализация модуля
int system_monitor_init(void);
void system_monitor_cleanup(void);
int system_monitor_start(void);
int system_monitor_stop(void);

// Получение информации о системных ресурсах
int get_cpu_info(cpu_info_t *info);
int get_memory_info(memory_info_t *info);
int get_system_monitor_info(system_info_t *info);

// Управление процессами
int get_system_process_list(system_process_info_t **processes, int *count);
int get_system_process_info(int pid, system_process_info_t *info);
void free_system_process_list(system_process_info_t *processes);

// Коллбэки для обновления данных
typedef void (*system_update_callback_t)(const cpu_info_t *cpu,
                                        const memory_info_t *memory,
                                        const system_info_t *system);

// Регистрация коллбэков
int register_system_update_callback(system_update_callback_t callback);
int unregister_system_update_callback(system_update_callback_t callback);

// Конфигурация модуля
int set_cpu_monitoring(bool enabled);
int set_memory_monitoring(bool enabled);
int set_process_monitoring(bool enabled);
int set_update_interval(int interval_ms);

// Получение состояния модуля
const system_monitor_state_t *get_system_monitor_state(void);

// Основная функция обновления метрик для главного цикла
int system_monitor_update_metrics(void);

// Макросы для безопасного использования
#define CHECK_SYSTEM_MONITOR_ENABLED() \
    if (!get_system_monitor_state()->cpu_monitor_enabled && \
        !get_system_monitor_state()->memory_monitor_enabled && \
        !get_system_monitor_state()->process_monitor_enabled) { \
        return -1; \
    }

#endif // SYSTEM_MONITOR_H