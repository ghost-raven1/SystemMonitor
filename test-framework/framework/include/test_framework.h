/**
 * @file test_framework.h
 * @brief Основной API тестового фреймворка SystemMonitor
 *
 * Тестовый фреймворк построен в соответствии с модульной архитектурой
 * SystemMonitor v3.0 и поддерживает следующие возможности:
 * - Модульное тестирование компонентов
 * - Интеграционное тестирование системных компонентов
 * - End-to-end тестирование полного функционала
 * - Систему событий для коммуникации между тестами
 * - Метрики производительности и покрытия
 * - Поддержку плагинов для расширения функционала
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

// Версии API
#define TEST_FRAMEWORK_VERSION_MAJOR 3
#define TEST_FRAMEWORK_VERSION_MINOR 0
#define TEST_FRAMEWORK_VERSION_PATCH 0

/**
 * @brief Типы тестов в соответствии с архитектурой SystemMonitor
 */
typedef enum {
    TEST_TYPE_UNIT = 1,          /**< Unit тесты для отдельных модулей */
    TEST_TYPE_INTEGRATION = 2,   /**< Интеграционные тесты между модулями */
    TEST_TYPE_SYSTEM = 3,        /**< Системные тесты платформенных компонентов */
    TEST_TYPE_END_TO_END = 4,    /**< End-to-end тесты полного функционала */
    TEST_TYPE_PERFORMANCE = 5,   /**< Тесты производительности */
    TEST_TYPE_STRESS = 6,        /**< Стресс-тесты */
    TEST_TYPE_REGRESSION = 7     /**< Регрессионные тесты */
} test_type_t;

/**
 * @brief Статусы выполнения теста
 */
typedef enum {
    TEST_STATUS_NOT_RUN = 0,     /**< Тест не выполнялся */
    TEST_STATUS_RUNNING = 1,     /**< Тест выполняется */
    TEST_STATUS_PASSED = 2,      /**< Тест пройден успешно */
    TEST_STATUS_FAILED = 3,      /**< Тест провален */
    TEST_STATUS_SKIPPED = 4,     /**< Тест пропущен */
    TEST_STATUS_ERROR = 5,       /**< Ошибка выполнения теста */
    TEST_STATUS_TIMEOUT = 6      /**< Тест превысил таймаут */
} test_status_t;

/**
 * @brief Приоритет теста для планировщика
 */
typedef enum {
    TEST_PRIORITY_LOW = 1,       /**< Низкий приоритет */
    TEST_PRIORITY_NORMAL = 2,    /**< Обычный приоритет */
    TEST_PRIORITY_HIGH = 3,      /**< Высокий приоритет */
    TEST_PRIORITY_CRITICAL = 4   /**< Критический приоритет */
} test_priority_t;

/**
 * @brief Структура для хранения результатов ассершена
 */
typedef struct {
    char expression[256];        /**< Выражение ассершена */
    char file[128];              /**< Файл с ассершеном */
    int line;                    /**< Номер строки */
    char message[512];           /**< Сообщение об ошибке */
    uint64_t timestamp;          /**< Временная метка */
    bool passed;                 /**< Результат ассершена */
} test_assertion_t;

/**
 * @brief Структура для хранения результатов выполнения теста
 */
typedef struct {
    char name[128];              /**< Имя теста */
    char suite[64];              /**< Группа тестов */
    test_type_t type;            /**< Тип теста */
    test_status_t status;        /**< Статус выполнения */
    test_priority_t priority;    /**< Приоритет теста */

    uint64_t start_time;         /**< Время начала выполнения */
    uint64_t end_time;           /**< Время завершения */
    uint64_t duration_ms;        /**< Длительность выполнения (мс) */

    int assertion_count;         /**< Количество ассершенов */
    int passed_assertions;       /**< Количество пройденных ассершенов */
    int failed_assertions;       /**< Количество проваленных ассершенов */

    test_assertion_t *assertions; /**< Массив ассершенов */
    size_t assertions_capacity;  /**< Размер массива ассершенов */

    char error_message[512];     /**< Сообщение об ошибке */
    char stdout_output[4096];    /**< Вывод теста в stdout */
    char stderr_output[4096];    /**< Вывод теста в stderr */

    void *test_data;             /**< Данные теста (для пользовательского использования) */
    void *user_data;             /**< Пользовательские данные */
} test_result_t;

/**
 * @brief Структура конфигурации тестового фреймворка
 */
typedef struct {
    bool verbose;                /**< Подробный вывод */
    bool colored_output;         /**< Цветной вывод */
    bool xml_output;             /**< Вывод в формате XML */
    bool json_output;            /**< Вывод в формате JSON */

    uint64_t default_timeout_ms; /**< Таймаут по умолчанию (мс) */
    uint32_t max_concurrent_tests; /**< Максимальное количество параллельных тестов */

    char output_dir[256];        /**< Директория для вывода результатов */
    char report_file[256];       /**< Файл отчета */

    bool enable_performance_monitoring; /**< Включить мониторинг производительности */
    bool enable_memory_tracking; /**< Включить отслеживание памяти */
    bool enable_coverage;        /**< Включить анализ покрытия */

    uint32_t random_seed;        /**< Сид для рандомизации тестов */
    bool shuffle_tests;          /**< Перемешивать порядок тестов */

    char *test_filter;           /**< Фильтр для выбора тестов */
    char *suite_filter;          /**< Фильтр для выбора групп тестов */
} test_config_t;

/**
 * @brief Структура контекста приложения (соответствует app_context из архитектуры)
 */
typedef struct {
    char app_name[64];           /**< Имя приложения */
    char app_version[16];        /**< Версия приложения */
    test_config_t config;        /**< Конфигурация тестирования */

    void *metrics_collector;     /**< Сборщик метрик (соответствует metrics_collector_t) */
    void *event_system;          /**< Система событий (соответствует event_system.h) */
    void *plugin_manager;        /**< Менеджер плагинов (соответствует plugin_manager_t) */

    uint64_t start_time;         /**< Время запуска фреймворка */
    uint32_t total_tests_run;    /**< Общее количество выполненных тестов */
    uint32_t total_assertions;   /**< Общее количество ассершенов */

    bool initialized;            /**< Инициализирован ли контекст */
} test_context_t;

/**
 * @brief Прототип функции теста
 */
typedef void (*test_function_t)(test_result_t *result, test_context_t *context);

/**
 * @brief Прототип функции настройки теста
 */
typedef void (*test_setup_t)(test_context_t *context);

/**
 * @brief Прототип функции очистки после теста
 */
typedef void (*test_teardown_t)(test_context_t *context);

/**
 * @brief Структура определения теста
 */
typedef struct {
    char name[128];              /**< Имя теста */
    char suite[64];              /**< Группа тестов */
    char description[256];       /**< Описание теста */
    char file[128];              /**< Файл с тестом */

    test_type_t type;            /**< Тип теста */
    test_priority_t priority;    /**< Приоритет теста */
    uint64_t timeout_ms;         /**< Таймаут теста (мс) */

    test_function_t test_func;   /**< Функция теста */
    test_setup_t setup_func;     /**< Функция настройки */
    test_teardown_t teardown_func; /**< Функция очистки */

    char *tags;                  /**< Теги теста (разделенные запятыми) */
    char *requirements;          /**< Требования для выполнения теста */

    void *test_data;             /**< Данные теста */
    void *user_data;             /**< Пользовательские данные */

    bool enabled;                /**< Включен ли тест */
    bool manual_only;            /**< Только ручной запуск */
} test_definition_t;

/**
 * @brief Структура группы тестов (test suite)
 */
typedef struct {
    char name[64];               /**< Имя группы тестов */
    char description[256];       /**< Описание группы */
    test_type_t type;            /**< Тип тестов в группе */

    test_definition_t *tests;    /**< Массив тестов */
    size_t test_count;           /**< Количество тестов */
    size_t test_capacity;        /**< Емкость массива тестов */

    test_setup_t suite_setup;    /**< Настройка для всей группы */
    test_teardown_t suite_teardown; /**< Очистка для всей группы */

    void *suite_data;            /**< Данные группы */
} test_suite_t;

/**
 * @brief Структура статистики тестирования
 */
typedef struct {
    uint32_t total_suites;       /**< Общее количество групп тестов */
    uint32_t total_tests;        /**< Общее количество тестов */
    uint32_t tests_run;          /**< Количество выполненных тестов */
    uint32_t tests_passed;       /**< Количество пройденных тестов */
    uint32_t tests_failed;       /**< Количество проваленных тестов */
    uint32_t tests_skipped;      /**< Количество пропущенных тестов */
    uint32_t tests_error;        /**< Количество тестов с ошибками */

    uint64_t total_duration_ms;  /**< Общая длительность тестирования */
    uint32_t total_assertions;   /**< Общее количество ассершенов */
    uint32_t passed_assertions;  /**< Количество пройденных ассершенов */
    uint32_t failed_assertions;  /**< Количество проваленных ассершенов */

    double success_rate;         /**< Процент успешных тестов */
    double avg_test_duration_ms; /**< Средняя длительность теста */
} test_statistics_t;

/**
 * @brief Структура плагина тестового фреймворка
 */
typedef struct {
    char name[64];               /**< Имя плагина */
    char version[16];            /**< Версия плагина */
    char description[256];       /**< Описание плагина */

    /* Операции плагина */
    int (*init)(test_context_t *context);                    /**< Инициализация плагина */
    int (*cleanup)(void);                                   /**< Очистка плагина */
    int (*process_result)(test_result_t *result);           /**< Обработка результата теста */
    int (*generate_report)(const char *format, char *output, size_t max_size); /**< Генерация отчета */

    void *plugin_data;           /**< Данные плагина */
    bool enabled;                /**< Включен ли плагин */
} test_plugin_t;

// Глобальные переменные фреймворка
extern test_context_t *global_test_context;
extern test_statistics_t *global_test_stats;

/* ============================================================================
 * Основные функции управления фреймворком
 * ============================================================================ */

/**
 * @brief Инициализация тестового фреймворка
 * @param config Конфигурация фреймворка
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_framework_init(const test_config_t *config);

/**
 * @brief Очистка ресурсов тестового фреймворка
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_framework_cleanup(void);

/**
 * @brief Запуск всех зарегистрированных тестов
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_framework_run_all(void);

/**
 * @brief Запуск тестов по фильтру
 * @param filter Фильтр для выбора тестов (может быть NULL для всех тестов)
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_framework_run_filtered(const char *filter);

/**
 * @brief Получение статистики тестирования
 * @param stats Структура для сохранения статистики
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_framework_get_statistics(test_statistics_t *stats);

/**
 * @brief Генерация отчета о тестировании
 * @param format Формат отчета ("xml", "json", "text")
 * @param output Буфер для сохранения отчета
 * @param max_size Максимальный размер буфера
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_framework_generate_report(const char *format, char *output, size_t max_size);

/* ============================================================================
 * Функции управления группами тестов (test suites)
 * ============================================================================ */

/**
 * @brief Создание новой группы тестов
 * @param name Имя группы
 * @param description Описание группы
 * @param type Тип тестов в группе
 * @return Указатель на созданную группу или NULL при ошибке
 */
test_suite_t *test_suite_create(const char *name, const char *description, test_type_t type);

/**
 * @brief Уничтожение группы тестов
 * @param suite Указатель на группу тестов
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_suite_destroy(test_suite_t *suite);

/**
 * @brief Добавление теста в группу
 * @param suite Группа тестов
 * @param test Определение теста
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_suite_add_test(test_suite_t *suite, const test_definition_t *test);

/**
 * @brief Запуск группы тестов
 * @param suite Группа тестов
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_suite_run(test_suite_t *suite);

/* ============================================================================
 * Функции управления отдельными тестами
 * ============================================================================ */

/**
 * @brief Регистрация нового теста
 * @param test_definition Определение теста
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_register(const test_definition_t *test_definition);

/**
 * @brief Создание определения теста
 * @param name Имя теста
 * @param suite_name Имя группы тестов
 * @param test_func Функция теста
 * @param type Тип теста
 * @return Указатель на созданное определение или NULL при ошибке
 */
test_definition_t *test_definition_create(const char *name,
                                        const char *suite_name,
                                        test_function_t test_func,
                                        test_type_t type);

/**
 * @brief Уничтожение определения теста
 * @param test_def Определение теста
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_definition_destroy(test_definition_t *test_def);

/* ============================================================================
 * Функции работы с контекстом
 * ============================================================================ */

/**
 * @brief Получение глобального контекста тестирования
 * @return Указатель на контекст или NULL при ошибке
 */
test_context_t *test_get_context(void);

/**
 * @brief Установка пользовательских данных в контекст
 * @param key Ключ для данных
 * @param data Указатель на данные
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_context_set_data(const char *key, void *data);

/**
 * @brief Получение пользовательских данных из контекста
 * @param key Ключ для данных
 * @return Указатель на данные или NULL при ошибке
 */
void *test_context_get_data(const char *key);

/* ============================================================================
 * Метрики и мониторинг производительности
 * ============================================================================ */

/**
 * @brief Структура метрик производительности (соответствует system_metrics_t)
 */
typedef struct {
    double cpu_usage_percent;    /**< Использование CPU (%) */
    uint64_t memory_used_bytes;  /**< Используемая память (байт) */
    uint64_t execution_time_ms;  /**< Время выполнения (мс) */
    uint32_t allocations_count;  /**< Количество аллокаций памяти */
    uint64_t peak_memory_usage;  /**< Пиковое использование памяти */
} test_metrics_t;

/**
 * @brief Получение метрик производительности для теста
 * @param test_name Имя теста
 * @param metrics Структура для сохранения метрик
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_get_metrics(const char *test_name, test_metrics_t *metrics);

/**
 * @brief Включение мониторинга производительности
 * @param enable Включить или отключить мониторинг
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_enable_performance_monitoring(bool enable);

/* ============================================================================
 * Система событий для интеграции с SystemMonitor
 * ============================================================================ */

/**
 * @brief Типы событий тестирования (соответствуют event_type_t из архитектуры)
 */
typedef enum {
    TEST_EVENT_STARTED = 100,    /**< Тест начат */
    TEST_EVENT_COMPLETED = 101,  /**< Тест завершен */
    TEST_EVENT_FAILED = 102,     /**< Тест провален */
    TEST_EVENT_SKIPPED = 103,    /**< Тест пропущен */
    TEST_EVENT_ASSERTION_FAILED = 104, /**< Ассершен провален */
    TEST_EVENT_PERFORMANCE_WARNING = 105, /**< Предупреждение производительности */
    TEST_EVENT_MEMORY_WARNING = 106,       /**< Предупреждение памяти */
    TEST_EVENT_SUITE_STARTED = 107,       /**< Группа тестов начата */
    TEST_EVENT_SUITE_COMPLETED = 108,     /**< Группа тестов завершена */
    TEST_EVENT_FRAMEWORK_READY = 109,     /**< Фреймворк готов */
    TEST_EVENT_FRAMEWORK_SHUTDOWN = 110   /**< Фреймворк завершает работу */
} test_event_type_t;

/**
 * @brief Регистрация обработчика событий
 * @param event_type Тип события
 * @param callback Функция обработчика
 * @param user_data Пользовательские данные
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_register_event_handler(test_event_type_t event_type,
                               void (*callback)(test_event_type_t, void *, void *),
                               void *user_data);

/**
 * @brief Отправка события
 * @param event_type Тип события
 * @param event_data Данные события
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int test_emit_event(test_event_type_t event_type, void *event_data);

/* ============================================================================
 * Утилитарные функции
 * ============================================================================ */

/**
 * @brief Генерация уникального имени для теста
 * @param base_name Базовое имя
 * @param buffer Буфер для сохранения результата
 * @param buffer_size Размер буфера
 */
void test_generate_unique_name(const char *base_name, char *buffer, size_t buffer_size);

/**
 * @brief Получение версии фреймворка
 * @param buffer Буфер для сохранения версии
 * @param buffer_size Размер буфера
 */
void test_framework_version(char *buffer, size_t buffer_size);

/**
 * @brief Включение/отключение цветного вывода
 * @param enable Включить цветной вывод
 */
void test_set_colored_output(bool enable);

/**
 * @brief Получение информации о системе (для тестов платформенных компонентов)
 * @return Строка с информацией о системе
 */
const char *test_get_system_info(void);

/* ============================================================================
 * Макросы для удобного создания тестов
 * ============================================================================ */

/**
 * @brief Макрос для объявления теста
 * @param test_name Имя теста
 * @param suite_name Имя группы
 */
#define TEST_DECL(test_name, suite_name) \
    void test_name(test_result_t *result, test_context_t *context)

/**
 * @brief Макрос для объявления группы тестов
 * @param suite_name Имя группы
 */
#define SUITE_DECL(suite_name) \
    test_suite_t *suite_name(void)

/**
 * @brief Макрос для регистрации теста (должен использоваться в функции main)
 * @param test_name Имя теста
 * @param suite_name Имя группы
 */
#define TEST_REGISTER(test_name, suite_name) \
    do { \
        test_definition_t *test_def = test_definition_create(#test_name, #suite_name, test_name, TEST_TYPE_UNIT); \
        if (test_def) { \
            test_register(test_def); \
        } \
    } while(0)

#endif // TEST_FRAMEWORK_H