/**
 * @file system_monitor_types.h
 * @brief Определения типов данных архитектуры SystemMonitor для тестового фреймворка
 *
 * Этот файл содержит определения структур данных и констант,
 * используемых в архитектуре SystemMonitor v3.0 для тестирования.
 */

#ifndef SYSTEM_MONITOR_TYPES_H
#define SYSTEM_MONITOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ============================================================================
 * Перечисления для архитектуры SystemMonitor
 * ============================================================================ */

/**
 * @brief Статусы модулей системы
 */
typedef enum {
    MODULE_STATUS_UNINITIALIZED = 0,
    MODULE_STATUS_INITIALIZING = 1,
    MODULE_STATUS_RUNNING = 2,
    MODULE_STATUS_STOPPING = 3,
    MODULE_STATUS_ERROR = 4,
    MODULE_STATUS_PAUSED = 5
} module_status_t;

/**
 * @brief Типы событий системы
 */
typedef enum {
    EVENT_MODULE_INITIALIZED = 1,
    EVENT_MODULE_FAILED = 2,
    EVENT_MODULE_DATA_UPDATED = 3,
    EVENT_PLATFORM_DATA_CHANGED = 4,
    EVENT_UI_THEME_CHANGED = 5,
    EVENT_CONFIG_UPDATED = 6,
    EVENT_ERROR_OCCURRED = 7,
    EVENT_PERFORMANCE_WARNING = 8,
    EVENT_SHUTDOWN_REQUESTED = 9,
    EVENT_USER_INPUT_RECEIVED = 10
} event_type_t;

/**
 * @brief Типы значений конфигурации
 */
typedef enum {
    CONFIG_TYPE_INT = 1,
    CONFIG_TYPE_DOUBLE = 2,
    CONFIG_TYPE_STRING = 3,
    CONFIG_TYPE_BOOL = 4,
    CONFIG_TYPE_ARRAY = 5
} config_value_type_t;

/**
 * @brief Типы плагинов системы
 */
typedef enum {
    PLUGIN_TYPE_MONITORING = 1,
    PLUGIN_TYPE_UI = 2,
    PLUGIN_TYPE_DATA_PROCESSING = 3,
    PLUGIN_TYPE_EXPORT = 4,
    PLUGIN_TYPE_NOTIFICATION = 5,
    PLUGIN_TYPE_INTEGRATION = 6
} plugin_type_t;

/**
 * @brief Типы платформ системы
 */
typedef enum {
    PLATFORM_UNKNOWN = 0,
    PLATFORM_MACOS = 1,
    PLATFORM_LINUX = 2,
    PLATFORM_WINDOWS = 3
} platform_type_t;

/**
 * @brief Константы типов платформ
 */
#define PLATFORM_MACOS 1
#define PLATFORM_LINUX 2
#define PLATFORM_WINDOWS 3

/* ============================================================================
 * Структуры данных метрик системы
 * ============================================================================ */

/**
 * @brief Структура метрики системы
 */
typedef struct {
    char name[64];                   /**< Имя метрики */
    char unit[16];                   /**< Единица измерения */
    double value;                    /**< Текущее значение */
    double min_value;                /**< Минимальное значение */
    double max_value;                /**< Максимальное значение */
    double average;                  /**< Среднее значение */
    uint64_t timestamp;              /**< Временная метка */
    int data_points;                 /**< Количество точек данных */
} metric_t;

/**
 * @brief Структура системных метрик
 */
typedef struct {
    double cpu_usage;                /**< Использование CPU (%) */
    double memory_usage;             /**< Использование памяти (%) */
    double disk_usage;               /**< Использование диска (%) */
    uint64_t network_rx;             /**< Получено данных (bytes/sec) */
    uint64_t network_tx;             /**< Отправлено данных (bytes/sec) */
    double temperature;              /**< Температура (°C) */
    double battery_level;            /**< Уровень батареи (%) */
    uint32_t process_count;          /**< Количество процессов */
    double load_average;             /**< Средняя нагрузка */
    uint64_t uptime;                 /**< Время работы (секунды) */
} system_metrics_t;

/* ============================================================================
 * Структуры данных модульной системы
 * ============================================================================ */

/**
 * @brief Структура информации о модуле
 */
typedef struct {
    char name[64];                   /**< Имя модуля */
    char version[16];                /**< Версия модуля */
    char description[256];           /**< Описание модуля */
    module_status_t status;          /**< Статус модуля */
    int priority;                    /**< Приоритет инициализации */
    bool required;                   /**< Критически важный модуль */
    void *private_data;              /**< Приватные данные модуля */
} module_info_t;

/**
 * @brief Структура операций модуля
 */
typedef struct {
    int (*init)(module_info_t *info, const void *ctx);   /**< Инициализация */
    int (*start)(void);                                  /**< Запуск */
    int (*stop)(void);                                   /**< Остановка */
    int (*pause)(void);                                  /**< Пауза */
    int (*resume)(void);                                 /**< Возобновление */
    int (*cleanup)(void);                                /**< Очистка */
    int (*get_data)(void *buffer, size_t buffer_size);   /**< Получение данных */
    int (*handle_event)(const char *event_type, void *event_data); /**< Обработка событий */
    const char *(*get_last_error)(void);                 /**< Последняя ошибка */
} module_operations_t;

/**
 * @brief Структура модуля системы
 */
typedef struct {
    module_info_t info;              /**< Информация о модуле */
    module_operations_t ops;         /**< Операции модуля */
} module_t;

/* ============================================================================
 * Структуры данных системы событий
 * ============================================================================ */

/**
 * @brief Структура события системы
 */
typedef struct {
    event_type_t type;               /**< Тип события */
    char source[64];                 /**< Источник события */
    uint64_t timestamp;              /**< Временная метка */
    void *data;                      /**< Данные события */
    size_t data_size;                /**< Размер данных */
} event_t;

/* ============================================================================
 * Структуры данных конфигурации
 * ============================================================================ */

/**
 * @brief Структура элемента конфигурации
 */
typedef struct {
    char key[128];                   /**< Ключ конфигурации */
    config_value_type_t type;        /**< Тип значения */
    union {
        int int_value;               /**< Целочисленное значение */
        double double_value;         /**< Значение с плавающей точкой */
        char string_value[256];      /**< Строковое значение */
        bool bool_value;             /**< Логическое значение */
        char *array_value;           /**< Массив (JSON строка) */
    } value;
    char description[256];           /**< Описание параметра */
    bool required;                   /**< Обязательный параметр */
} config_entry_t;

/* ============================================================================
 * Структуры данных плагинов
 * ============================================================================ */

/**
 * @brief Структура информации о плагине
 */
typedef struct {
    char name[64];                   /**< Имя плагина */
    char version[16];                /**< Версия плагина */
    char description[256];           /**< Описание плагина */
    plugin_type_t type;              /**< Тип плагина */
    char author[64];                 /**< Автор плагина */
    char license[64];                /**< Лицензия */
    char *dependencies;              /**< Зависимости (JSON) */
    bool enabled;                    /**< Включен ли плагин */
    int load_order;                  /**< Порядок загрузки */
} plugin_info_t;

/**
 * @brief Структура интерфейса плагина
 */
typedef struct {
    /* Базовые операции плагина */
    int (*init)(const plugin_info_t *info, const void *ctx);         /**< Инициализация */
    int (*start)(void);                                              /**< Запуск */
    int (*stop)(void);                                               /**< Остановка */
    int (*cleanup)(void);                                            /**< Очистка */
    int (*get_plugin_info)(plugin_info_t *info);                     /**< Информация о плагине */
    int (*handle_event)(const event_t *event);                       /**< Обработка событий */
    const char *(*get_last_error)(void);                             /**< Последняя ошибка */
    int (*validate_config)(const void *config);                     /**< Валидация конфигурации */
} plugin_interface_t;

/**
 * @brief Структура плагина системы
 */
typedef struct {
    plugin_info_t info;              /**< Информация о плагине */
    plugin_interface_t interface;    /**< Интерфейс плагина */
    void *handle;                    /**< Дескриптор динамической библиотеки */
    char file_path[256];             /**< Путь к файлу плагина */
    time_t load_time;                /**< Время загрузки */
    bool initialized;                /**< Инициализирован ли плагин */
} plugin_t;

/* ============================================================================
 * Структура контекста приложения
 * ============================================================================ */

/**
 * @brief Структура контекста приложения
 */
typedef struct {
    char app_name[64];               /**< Имя приложения */
    char app_version[16];            /**< Версия приложения */
    void *metrics_collector;         /**< Сборщик метрик */
    void *event_system;              /**< Система событий */
    void *plugin_manager;            /**< Менеджер плагинов */
    uint64_t start_time;             /**< Время запуска */
    bool initialized;                /**< Инициализирован ли контекст */
} app_context_t;

/* ============================================================================
 * Макросы для безопасной работы со строками
 * ============================================================================ */

/**
 * @brief Безопасное копирование строк
 */
#define safe_strcpy(dest, src, size) \
    do { \
        if (dest && src && size > 0) { \
            strncpy(dest, src, size - 1); \
            dest[size - 1] = '\0'; \
        } \
    } while(0)

/* ============================================================================
 * Структуры данных платформенных модулей
 * ============================================================================ */

/**
 * @brief Структура информации о платформе
 */
typedef struct {
    char platform_name[64];          /**< Имя платформы */
    char version[32];                /**< Версия ОС */
    char architecture[32];           /**< Архитектура */
    uint32_t cpu_cores;              /**< Количество ядер CPU */
    uint64_t total_memory;           /**< Общий объем памяти */
    char kernel_version[64];          /**< Версия ядра */
} platform_info_t;

/**
 * @brief Структура информации о процессе
 */
typedef struct {
    int32_t pid;                     /**< ID процесса */
    char name[256];                  /**< Имя процесса */
    char user[64];                   /**< Пользователь */
    float cpu_usage;                 /**< Использование CPU (%) */
    float mem_usage;                 /**< Использование памяти (%) */
    uint64_t memory_bytes;           /**< Память в байтах */
    uint64_t start_time;             /**< Время запуска */
    char state[16];                  /**< Состояние процесса */
} process_info_t;

/**
 * @brief Структура информации о GPU
 */
typedef struct {
    char name[128];                  /**< Имя GPU */
    char driver[64];                 /**< Драйвер */
    float temperature;               /**< Температура (°C) */
    int memory_mb;                   /**< Память (MB) */
    float usage_percent;             /**< Использование (%) */
    uint32_t clock_speed;            /**< Частота (MHz) */
} gpu_info_t;

/**
 * @brief Структура статистики GPU
 */
typedef struct {
    uint64_t memory_used;            /**< Используемая память */
    uint64_t memory_total;           /**< Общая память */
    float utilization_percent;       /**< Утилизация (%) */
    uint32_t temperature;            /**< Температура */
    uint32_t fan_speed;              /**< Скорость вентилятора */
} gpu_stats_t;

/**
 * @brief Структура информации о порту
 */
typedef struct {
    uint16_t port;                   /**< Номер порта */
    char protocol[8];                /**< Протокол (TCP/UDP) */
    char process_name[256];          /**< Имя процесса */
    int32_t pid;                     /**< PID процесса */
    char state[16];                  /**< Состояние порта */
} port_info_t;

/* ============================================================================
 * Структуры данных для инструментов разработки
 * ============================================================================ */

/**
 * @brief Структура окружения разработки
 */
typedef struct {
    char ide_name[64];               /**< Имя IDE */
    char languages[256];             /**< Языки программирования */
    char frameworks[256];            /**< Фреймворки */
    char tools[512];                 /**< Инструменты разработки */
    bool debugging_enabled;          /**< Отладка включена */
} dev_environment_t;

/**
 * @brief Структура процесса разработки
 */
typedef struct {
    int32_t pid;                     /**< PID процесса */
    char name[256];                  /**< Имя процесса */
    char type[32];                   /**< Тип (IDE, Compiler, Debugger) */
    float cpu_usage;                 /**< Использование CPU (%) */
    float mem_usage;                 /**< Использование памяти (%) */
    uint64_t memory_bytes;           /**< Память в байтах */
    uint64_t start_time;             /**< Время запуска */
} dev_process_t;

/**
 * @brief Структура метрик разработки
 */
typedef struct {
    uint32_t process_count;          /**< Количество процессов разработки */
    uint32_t port_count;             /**< Количество портов разработки */
    float total_cpu_usage;           /**< Общее использование CPU (%) */
    float total_memory_usage;        /**< Общее использование памяти (%) */
    uint64_t total_memory_bytes;     /**< Общая память в байтах */
} dev_metrics_t;

/**
 * @brief Структура производительности разработки
 */
typedef struct {
    double avg_build_time_sec;       /**< Среднее время сборки (сек) */
    double memory_usage_mb;          /**< Использование памяти (MB) */
    float cpu_load_percent;          /**< Загрузка CPU (%) */
    uint32_t compilation_errors;     /**< Ошибки компиляции */
    uint32_t test_failures;          /**< Проваленные тесты */
} dev_performance_t;

/**
 * @brief Структура информации об IDE
 */
typedef struct {
    char name[64];                   /**< Имя IDE */
    char version[32];                /**< Версия IDE */
    char workspace[256];             /**< Рабочая директория */
    bool is_running;                 /**< IDE запущена */
    uint32_t open_files;             /**< Открытых файлов */
} ide_info_t;

/* ============================================================================
 * Структуры данных для диагностики
 * ============================================================================ */

/**
 * @brief Структура результата диагностики
 */
typedef struct {
    char check_name[128];            /**< Имя проверки */
    char description[256];           /**< Описание проблемы */
    int severity;                    /**< Уровень серьезности (0-3) */
    uint64_t timestamp;              /**< Временная метка */
    char recommendation[512];        /**< Рекомендация по исправлению */
    void *details;                   /**< Детали проблемы */
} diagnostic_result_t;

/**
 * @brief Структура конфигурации диагностики
 */
typedef struct {
    bool enable_system_checks;       /**< Включить системные проверки */
    bool enable_performance_checks;  /**< Включить проверки производительности */
    bool enable_security_checks;     /**< Включить проверки безопасности */
    bool enable_network_checks;      /**< Включить сетевые проверки */
    uint32_t check_interval_ms;      /**< Интервал проверок (мс) */
    uint32_t max_log_entries;        /**< Максимум записей в логе */
    char log_file[256];              /**< Файл лога диагностики */
} diagnostic_config_t;

/* ============================================================================
 * Структуры данных для System Monitor модуля
 * ============================================================================ */

/**
 * @brief Структура конфигурации System Monitor
 */
typedef struct {
    uint32_t update_interval_ms;     /**< Интервал обновления (мс) */
    bool enabled;                    /**< Включен мониторинг */
    uint32_t max_history_entries;    /**< Максимум записей истории */
    char output_format[32];          /**< Формат вывода */
} system_monitor_config_t;

/* ============================================================================
 * Структуры данных для батареи (используются оригинальные из платформенных модулей)
 * ============================================================================ */

/**
 * @brief Структура разбивки памяти
 */
typedef struct {
    uint64_t total;                  /**< Общая память */
    uint64_t used;                   /**< Используемая память */
    uint64_t free;                   /**< Свободная память */
    uint64_t available;              /**< Доступная память */
    uint64_t buffers;                /**< Буферы */
    uint64_t cached;                 /**< Кэш */
} memory_breakdown_t;

#endif // SYSTEM_MONITOR_TYPES_H