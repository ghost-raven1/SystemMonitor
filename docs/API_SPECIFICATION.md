# Спецификация нового API для SystemMonitor v3.0

## Обзор архитектуры

Новая архитектура SystemMonitor основана на четком разделении ответственностей между модулями с использованием современных компонентов:

- **app_context** - централизованное управление состоянием
- **ui_renderer** - модульный рендерер интерфейса
- **enhanced.c** - расширенные возможности мониторинга
- **modern_ui_demo** - система современных тем

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                           │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  Core       │ │  Modules    │ │  Platform   │ │  UI         │ │
│  │             │ │             │ │             │ │             │ │
│  │ • app_context│ • system_   │ • battery   │ • ui_renderer│ │
│  │ • app_events│   monitor   │ • disk      │ • ui_theme  │ │
│  │ • developer │ • grafana   │ • network   │ • ui_window_│ │
│  │ • process_  │ • prometheus│ • processes │   manager   │ │
│  │   tree      │ • diagnostics│ • gpu       │ • modern_ui │ │
│  └─────────────┘ └─────────────┘ • system_   │ └─────────────┘ │
│                                   info       │                 │
│                                   • usb      │                 │
│                                   • smart    │                 │
│                                  └─────────────┘               │
├─────────────────────────────────────────────────────────────────┤
│                    Utilities Layer                             │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  Config     │ │  Enhanced   │ │  Error      │ │  Logging    │ │
│  │             │ │             │ │  Handler    │ │             │ │
│  │ • config.c  │ • enhanced.c│ • error_    │ • logging.c │ │
│  │ • config.h  │ • enhanced.h│   handler.c │ • logging.h │ │
│  │             │ │             │ • error_    │ │             │
│  │             │ │             │   handler.h │ │             │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 1. Интерфейсы между модулями

### 1.1 Контракт между Core и Modules

```c
// src/core/module_interface.h
#ifndef MODULE_INTERFACE_H
#define MODULE_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MODULE_STATUS_UNINITIALIZED = 0,
    MODULE_STATUS_INITIALIZING = 1,
    MODULE_STATUS_RUNNING = 2,
    MODULE_STATUS_STOPPING = 3,
    MODULE_STATUS_ERROR = 4,
    MODULE_STATUS_PAUSED = 5
} module_status_t;

typedef struct {
    char name[64];
    char version[16];
    char description[256];
    module_status_t status;
    int priority;                    // Приоритет инициализации
    bool required;                   // Критически важный модуль
    void *private_data;              // Приватные данные модуля
} module_info_t;

typedef struct {
    int (*init)(module_info_t *info, const app_context_t *ctx);
    int (*start)(void);
    int (*stop)(void);
    int (*pause)(void);
    int (*resume)(void);
    int (*cleanup)(void);
    int (*get_data)(void *buffer, size_t buffer_size);
    int (*handle_event)(const char *event_type, void *event_data);
    const char *(*get_last_error)(void);
} module_operations_t;

typedef struct {
    module_info_t info;
    module_operations_t ops;
} module_t;

// Регистрация модуля в системе
int module_register(const module_t *module);
int module_unregister(const char *module_name);

// Управление жизненным циклом модулей
int module_initialize_all(void);
int module_start_all(void);
int module_stop_all(void);
int module_cleanup_all(void);

// Получение информации о модулях
int module_get_info(const char *module_name, module_info_t *info);
int module_get_status(const char *module_name, module_status_t *status);
int module_list_all(char *buffer, size_t buffer_size);

#endif // MODULE_INTERFACE_H
```

### 1.2 Контракт между Platform и Core

```c
// src/platform/platform_interface.h
#ifndef PLATFORM_INTERFACE_H
#define PLATFORM_INTERFACE_H

#include <stdint.h>

typedef enum {
    PLATFORM_MACOS = 1,
    PLATFORM_LINUX = 2,
    PLATFORM_WINDOWS = 3,
    PLATFORM_BSD = 4
} platform_type_t;

typedef struct {
    platform_type_t type;
    char name[64];
    char version[32];
    bool supported;                  // Поддерживается ли платформа
} platform_info_t;

typedef struct {
    int (*get_cpu_info)(cpu_info_t *info);
    int (*get_memory_info)(memory_info_t *info);
    int (*get_disk_info)(disk_info_t *info);
    int (*get_network_info)(network_info_t *info);
    int (*get_process_list)(process_info_t *processes, int max_count);
    int (*get_battery_info)(battery_info_t *info);
    int (*get_gpu_info)(gpu_info_t *info);
    int (*get_system_info)(system_info_t *info);
    int (*execute_command)(const char *cmd, char *output, size_t max_len);
} platform_operations_t;

typedef struct {
    platform_info_t info;
    platform_operations_t ops;
} platform_adapter_t;

#endif // PLATFORM_INTERFACE_H
```

## 2. Система событий и коллбеков

### 2.1 Централизованная система событий

```c
// src/core/event_system.h
#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

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

typedef struct {
    event_type_t type;
    char source[64];                 // Источник события
    uint64_t timestamp;              // Временная метка
    void *data;                      // Данные события
    size_t data_size;                // Размер данных
} event_t;

typedef void (*event_callback_t)(const event_t *event, void *user_data);

// Регистрация обработчиков событий
int event_register_handler(event_type_t event_type,
                          event_callback_t callback,
                          void *user_data,
                          int priority);

int event_unregister_handler(event_type_t event_type,
                            event_callback_t callback);

// Отправка событий
int event_emit(event_type_t event_type,
               const char *source,
               void *data,
               size_t data_size);

// Асинхронная обработка событий
int event_process_async(const event_t *event);

#endif // EVENT_SYSTEM_H
```

## 3. Структуры данных для обмена информацией

### 3.1 Стандартизированные структуры метрик

```c
// src/core/metrics.h
#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <time.h>

typedef struct {
    char name[64];                   // Имя метрики
    char unit[16];                   // Единица измерения
    double value;                    // Текущее значение
    double min_value;                // Минимальное значение
    double max_value;                // Максимальное значение
    double average;                  // Среднее значение
    uint64_t timestamp;              // Временная метка
    int data_points;                 // Количество точек данных
} metric_t;

typedef struct {
    metric_t cpu_usage;              // Использование CPU (%)
    metric_t memory_usage;           // Использование памяти (%)
    metric_t disk_usage;             // Использование диска (%)
    metric_t network_rx;             // Получено данных (bytes/sec)
    metric_t network_tx;             // Отправлено данных (bytes/sec)
    metric_t temperature;            // Температура (°C)
    metric_t battery_level;          // Уровень батареи (%)
    metric_t process_count;          // Количество процессов
    metric_t load_average;           // Средняя нагрузка
    metric_t uptime;                 // Время работы (секунды)
} system_metrics_t;

typedef struct {
    system_metrics_t current;        // Текущие метрики
    system_metrics_t previous;       // Предыдущие метрики
    system_metrics_t *history;       // История метрик (циклический буфер)
    int history_size;                // Размер истории
    int history_index;               // Текущий индекс в истории
} metrics_collector_t;

#endif // METRICS_H
```

### 3.2 Структура данных мониторинга

```c
// src/modules/monitoring_interface.h
#ifndef MONITORING_INTERFACE_H
#define MONITORING_INTERFACE_H

typedef struct {
    char name[64];                   // Имя процесса
    int pid;                         // PID процесса
    char user[32];                   // Пользователь
    char state[16];                  // Состояние процесса
    double cpu_percent;              // Использование CPU (%)
    uint64_t memory_bytes;           // Использование памяти (байт)
    uint64_t start_time;             // Время запуска
    char command[256];               // Команда запуска
} process_info_t;

typedef struct {
    uint64_t total_bytes;            // Общий размер
    uint64_t used_bytes;             // Используемый размер
    uint64_t free_bytes;             // Свободный размер
    char mount_point[256];           // Точка монтирования
    char filesystem[64];             // Тип файловой системы
    int usage_percent;               // Процент использования
} disk_info_t;

typedef struct {
    char interface_name[32];         // Имя интерфейса
    char ip_address[46];             // IP адрес (IPv4/IPv6)
    uint64_t rx_bytes;               // Получено байт
    uint64_t tx_bytes;               // Отправлено байт
    uint64_t rx_packets;             // Получено пакетов
    uint64_t tx_packets;             // Отправлено пакетов
    int status;                      // Статус интерфейса
} network_interface_t;

#endif // MONITORING_INTERFACE_H
```

## 4. Интерфейсы модулей мониторинга

### 4.1 CPU мониторинг

```c
// src/modules/cpu_monitor.h
#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

typedef struct {
    int core_count;                  // Количество ядер
    int thread_count;                // Количество потоков
    char model_name[128];            // Название модели
    uint64_t frequency_mhz;          // Частота (MHz)
    double temperature_celsius;      // Температура (°C)
    double load_percent;             // Нагрузка (%)
    double *core_usage;              // Использование по ядрам
} cpu_info_t;

typedef struct {
    module_t base;                   // Базовый интерфейс модуля

    // Специфичные операции CPU мониторинга
    int (*get_cpu_info)(cpu_info_t *info);
    int (*get_cpu_usage)(double *usage_percent);
    int (*get_core_temperatures)(double *temperatures, int max_cores);
    int (*get_cpu_frequency)(uint64_t *frequency_mhz);
    int (*set_cpu_governor)(const char *governor);

    // Расширенные функции
    int (*get_cpu_history)(double *history, int max_points);
    int (*get_top_processes)(process_info_t *processes, int max_count);
    int (*enable_cpu_monitoring)(bool enable);
} cpu_monitor_t;

#endif // CPU_MONITOR_H
```

### 4.2 Мониторинг памяти

```c
// src/modules/memory_monitor.h
#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

typedef struct {
    uint64_t total_bytes;            // Общий объем памяти
    uint64_t used_bytes;             // Используемая память
    uint64_t free_bytes;             // Свободная память
    uint64_t cached_bytes;           // Кэшированная память
    uint64_t buffers_bytes;          // Буферы
    uint64_t swap_total_bytes;       // Общий объем swap
    uint64_t swap_used_bytes;        // Используемый swap
    uint64_t swap_free_bytes;        // Свободный swap
} memory_info_t;

typedef struct {
    module_t base;                   // Базовый интерфейс модуля

    // Специфичные операции мониторинга памяти
    int (*get_memory_info)(memory_info_t *info);
    int (*get_memory_usage)(double *usage_percent);
    int (*get_swap_info)(memory_info_t *swap_info);
    int (*get_memory_pressure)(int *pressure_level);
    int (*clear_memory_cache)(void);

    // Расширенные функции
    int (*get_memory_history)(double *history, int max_points);
    int (*get_memory_leaks)(process_info_t *leaky_processes, int max_count);
    int (*enable_memory_monitoring)(bool enable);
} memory_monitor_t;

#endif // MEMORY_MONITOR_H
```

## 5. Система конфигурации и тем

### 5.1 Управление конфигурацией

```c
// src/utils/config_manager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

typedef enum {
    CONFIG_TYPE_INT = 1,
    CONFIG_TYPE_DOUBLE = 2,
    CONFIG_TYPE_STRING = 3,
    CONFIG_TYPE_BOOL = 4,
    CONFIG_TYPE_ARRAY = 5
} config_value_type_t;

typedef struct {
    char key[128];                   // Ключ конфигурации
    config_value_type_t type;        // Тип значения
    union {
        int int_value;
        double double_value;
        char string_value[256];
        bool bool_value;
        char *array_value;           // JSON строка для массивов
    } value;
    char description[256];           // Описание параметра
    bool required;                   // Обязательный параметр
} config_entry_t;

typedef struct {
    char name[64];                   // Имя конфигурации
    char version[16];                // Версия схемы
    config_entry_t *entries;         // Массив параметров
    int entry_count;                 // Количество параметров
    char file_path[256];             // Путь к файлу конфигурации
    bool auto_save;                  // Автосохранение изменений
} config_schema_t;

typedef struct {
    config_schema_t schema;          // Схема конфигурации

    // Операции управления конфигурацией
    int (*load_from_file)(const char *file_path);
    int (*save_to_file)(const char *file_path);
    int (*get_value)(const char *key, config_entry_t *entry);
    int (*set_value)(const char *key, const config_entry_t *entry);
    int (*reset_to_defaults)(void);
    int (*validate_config)(void);

    // Расширенные функции
    int (*export_config)(const char *format, char *output, size_t max_size);
    int (*import_config)(const char *format, const char *data);
    int (*get_config_history)(config_entry_t *history, int max_entries);
} config_manager_t;

#endif // CONFIG_MANAGER_H
```

### 5.2 Система тем на основе modern_ui_demo

```c
// src/ui/theme_system.h
#ifndef THEME_SYSTEM_H
#define THEME_SYSTEM_H

typedef enum {
    THEME_BTOP_DARK = 1,             // Темная тема в стиле btop
    THEME_BTOP_NEON = 2,             // Неоновая тема
    THEME_BTOP_MATRIX = 3,           // Матричная тема
    THEME_BTOP_LIGHT = 4,            // Светлая тема
    THEME_MODERN_MINIMAL = 5,        // Минималистичная тема
    THEME_HIGH_CONTRAST = 6          // Высококонтрастная тема
} theme_type_t;

typedef struct {
    char name[32];                   // Название темы
    theme_type_t type;               // Тип темы
    bool is_dark;                    // Темная тема

    // Цветовая палитра
    struct {
        int background;              // Фон
        int foreground;              // Основной текст
        int accent;                  // Акцентный цвет
        int success;                 // Успех
        int warning;                 // Предупреждение
        int error;                   // Ошибка
        int info;                    // Информация
        int cpu_low;                 // Низкая нагрузка CPU
        int cpu_medium;              // Средняя нагрузка CPU
        int cpu_high;                // Высокая нагрузка CPU
        int memory_low;              // Низкое использование памяти
        int memory_medium;           // Среднее использование памяти
        int memory_high;             // Высокое использование памяти
        int network_low;             // Низкая активность сети
        int network_medium;          // Средняя активность сети
        int network_high;            // Высокая активность сети
    } colors;

    // Шрифтовые настройки
    struct {
        int font_size;               // Размер шрифта
        bool bold_headers;           // Жирные заголовки
        bool italic_comments;        // Курсивные комментарии
        char font_family[64];        // Семейство шрифтов
    } fonts;

    // Параметры компоновки
    struct {
        int spacing;                 // Расстояние между элементами
        int padding;                 // Внутренние отступы
        int border_width;            // Ширина рамок
        bool show_icons;             // Показывать иконки
        bool show_borders;           // Показывать рамки
        bool enable_animations;      // Включить анимации
    } layout;

} ui_theme_t;

typedef struct {
    ui_theme_t current_theme;        // Текущая тема

    // Операции управления темами
    int (*load_theme)(theme_type_t theme_type);
    int (*load_theme_from_file)(const char *file_path);
    int (*save_theme_to_file)(const char *file_path);
    int (*get_available_themes)(ui_theme_t *themes, int max_count);
    int (*create_custom_theme)(const ui_theme_t *base_theme, const char *name);
    int (*delete_custom_theme)(const char *theme_name);

    // Работа с цветами
    int (*get_color)(const char *color_name, int *color_value);
    int (*set_color)(const char *color_name, int color_value);
    int (*reset_theme_to_default)(theme_type_t theme_type);

    // Предварительный просмотр
    int (*preview_theme)(theme_type_t theme_type);
    int (*apply_theme)(theme_type_t theme_type);

    // Анимации и эффекты
    int (*set_animation_enabled)(bool enabled);
    int (*set_animation_speed)(double speed_multiplier);
    int (*get_theme_info)(theme_type_t theme_type, ui_theme_t *theme_info);

} theme_manager_t;

#endif // THEME_SYSTEM_H
```

## 6. API расширений и плагинов

### 6.1 Система плагинов на основе enhanced функций

```c
// src/plugins/plugin_system.h
#ifndef PLUGIN_SYSTEM_H
#define PLUGIN_SYSTEM_H

typedef enum {
    PLUGIN_TYPE_MONITORING = 1,      // Мониторинг
    PLUGIN_TYPE_UI = 2,              // Пользовательский интерфейс
    PLUGIN_TYPE_DATA_PROCESSING = 3, // Обработка данных
    PLUGIN_TYPE_EXPORT = 4,          // Экспорт данных
    PLUGIN_TYPE_NOTIFICATION = 5,    // Уведомления
    PLUGIN_TYPE_INTEGRATION = 6      // Интеграция со внешними сервисами
} plugin_type_t;

typedef struct {
    char name[64];                   // Имя плагина
    char version[16];                // Версия плагина
    char description[256];           // Описание плагина
    plugin_type_t type;              // Тип плагина
    char author[64];                 // Автор плагина
    char license[64];                // Лицензия
    char *dependencies;              // Зависимости (JSON)
    bool enabled;                    // Включен ли плагин
    int load_order;                  // Порядок загрузки
} plugin_info_t;

typedef struct {
    // Базовые операции плагина
    int (*init)(const plugin_info_t *info, const app_context_t *ctx);
    int (*start)(void);
    int (*stop)(void);
    int (*cleanup)(void);

    // Специфичные операции в зависимости от типа
    union {
        // Мониторинг
        struct {
            int (*collect_metrics)(metric_t *metrics, int max_count);
            int (*get_custom_data)(void *buffer, size_t buffer_size);
            int (*process_data)(const void *input, void *output);
        } monitoring;

        // Пользовательский интерфейс
        struct {
            int (*render_custom_ui)(int start_y, int start_x, int height, int width);
            int (*handle_input)(int key, void *user_data);
            int (*get_ui_config)(void *config);
        } ui;

        // Экспорт данных
        struct {
            int (*export_data)(const void *data, const char *format, const char *file_path);
            int (*get_supported_formats)(char *formats, size_t max_size);
            int (*validate_export_config)(const void *config);
        } export_plugin;

        // Уведомления
        struct {
            int (*send_notification)(const char *title, const char *message, int priority);
            int (*get_notification_settings)(void *settings);
            int (*test_notification)(void);
        } notification;

        // Интеграция
        struct {
            int (*connect_service)(const void *config);
            int (*disconnect_service)(void);
            int (*sync_data)(void *data, size_t *data_size);
        } integration;
    } operations;

    // Общие операции
    int (*get_plugin_info)(plugin_info_t *info);
    int (*handle_event)(const event_t *event);
    const char *(*get_last_error)(void);
    int (*validate_config)(const void *config);

} plugin_interface_t;

typedef struct {
    plugin_info_t info;
    plugin_interface_t interface;
    void *handle;                    // Дескриптор динамической библиотеки
    char file_path[256];             // Путь к файлу плагина
    time_t load_time;                // Время загрузки
    bool initialized;                // Инициализирован ли плагин
} plugin_t;

typedef struct {
    // Управление плагинами
    int (*load_plugin)(const char *plugin_path);
    int (*unload_plugin)(const char *plugin_name);
    int (*enable_plugin)(const char *plugin_name, bool enable);
    int (*reload_plugin)(const char *plugin_name);

    // Информация о плагинах
    int (*get_plugin_list)(plugin_info_t *plugins, int max_count);
    int (*get_plugin_info)(const char *plugin_name, plugin_info_t *info);
    int (*get_plugin_status)(const char *plugin_name, bool *enabled);

    // Конфигурация плагинов
    int (*configure_plugin)(const char *plugin_name, const void *config);
    int (*get_plugin_config)(const char *plugin_name, void *config);

    // События плагинов
    int (*broadcast_event)(const event_t *event);
    int (*register_event_handler)(const char *plugin_name,
                                 event_type_t event_type,
                                 event_callback_t callback);

    // Безопасность плагинов
    int (*validate_plugin)(const char *plugin_path);
    int (*set_plugin_permissions)(const char *plugin_name, const char *permissions);
    int (*check_plugin_permissions)(const char *plugin_name, const char *permission);

} plugin_manager_t;

#endif // PLUGIN_SYSTEM_H
```

## 7. Стратегия миграции

### 7.1 Этапы миграции

```
graph TD
    A[Фаза 1: Подготовка] --> B[Фаза 2: Модуляризация]
    B --> C[Фаза 3: Интеграция]
    C --> D[Фаза 4: Оптимизация]
    D --> E[Фаза 5: Деплой]

    A --> A1[Анализ зависимостей]
    A --> A2[Создание интерфейсов]
    A --> A3[Тестирование миграции]

    B --> B1[Выделение UI рендерера]
    B --> B2[Выделение бизнес-логики]
    B --> B3[Создание адаптеров]

    C --> C1[Интеграция app_context]
    C --> C2[Подключение системы событий]
    C --> C3[Настройка конфигурации]

    D --> D1[Оптимизация производительности]
    D --> D2[Добавление кэширования]
    D --> D3[Асинхронная обработка]

    E --> E1[Тестирование в продакшене]
    E --> E2[Мониторинг стабильности]
    E --> E3[Финальная миграция]
```

### 7.2 Механизм обратной совместимости

```c
// src/legacy/compatibility_layer.h
#ifndef COMPATIBILITY_LAYER_H
#define COMPATIBILITY_LAYER_H

typedef struct {
    bool legacy_mode_enabled;        // Включен ли режим совместимости
    char legacy_ui_file[256];        // Путь к legacy UI
    int compatibility_version;       // Версия совместимости

    // Функции обратной совместимости
    int (*translate_legacy_config)(const char *legacy_config, char *new_config);
    int (*migrate_legacy_data)(const char *legacy_data_path, const char *new_data_path);
    int (*emulate_legacy_ui)(const char *ui_commands, char *output);

    // Адаптеры для старых интерфейсов
    int (*legacy_ui_init)(void);
    int (*legacy_ui_render)(const system_metrics_t *metrics);
    int (*legacy_ui_handle_input)(int key);

} compatibility_layer_t;

// Глобальные функции совместимости
int enable_legacy_mode(const char *legacy_ui_path);
int disable_legacy_mode(void);
bool is_legacy_mode_active(void);

#endif // COMPATIBILITY_LAYER_H
```

## Заключение

Предлагаемая архитектура нового API обеспечивает:

1. **Модульность** - четкое разделение ответственностей
2. **Расширяемость** - система плагинов и расширений
3. **Современность** - использование современных компонентов
4. **Надежность** - система событий и обработка ошибок
5. **Производительность** - асинхронная обработка и кэширование
6. **Совместимость** - плавная миграция от legacy кода

Следующие шаги:
1. Создать базовые интерфейсы и структуры данных
2. Реализовать систему событий и коллбеков
3. Разработать план миграции с механизмами отката
4. Начать поэтапное внедрение новых компонентов