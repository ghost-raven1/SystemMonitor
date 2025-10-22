/**
 * @file test_framework.c
 * @brief Реализация основного API тестового фреймворка
 *
 * Этот файл содержит реализацию функций тестового фреймворка,
 * интегрированного с архитектурой SystemMonitor v3.0.
 */

#include "test_framework.h"
#include "test_asserts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

/* ============================================================================
 * Глобальные переменные
 * ============================================================================ */

static test_context_t global_context = {0};
static test_statistics_t global_statistics = {0};
static test_suite_t *test_suites[256] = {0};
static size_t suite_count = 0;
static pthread_mutex_t framework_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool framework_initialized = false;

/* Структура для обработчиков событий */
typedef struct event_handler {
    test_event_type_t event_type;
    void (*callback)(test_event_type_t, void *, void *);
    void *user_data;
    struct event_handler *next;
} event_handler_t;

static event_handler_t *event_handlers = NULL;

/* ============================================================================
 * Внутренние функции
 * ============================================================================ */

/**
 * @brief Получение текущего времени в миллисекундах
 */
static uint64_t get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

/**
 * @brief Безопасное копирование строк
 */
static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest && src && dest_size > 0) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

/**
 * @brief Поиск группы тестов по имени
 */
static test_suite_t *find_suite_by_name(const char *name) {
    for (size_t i = 0; i < suite_count; ++i) {
        if (test_suites[i] && strcmp(test_suites[i]->name, name) == 0) {
            return test_suites[i];
        }
    }
    return NULL;
}

/**
 * @brief Добавление группы тестов в массив
 */
static int add_suite_to_array(test_suite_t *suite) {
    pthread_mutex_lock(&framework_mutex);

    if (suite_count >= sizeof(test_suites) / sizeof(test_suites[0])) {
        pthread_mutex_unlock(&framework_mutex);
        return -1;
    }

    test_suites[suite_count++] = suite;
    pthread_mutex_unlock(&framework_mutex);
    return 0;
}

/**
 * @brief Создание результата теста
 */
static test_result_t *create_test_result(const test_definition_t *test_def) {
    test_result_t *result = calloc(1, sizeof(test_result_t));
    if (!result) {
        return NULL;
    }

    safe_strcpy(result->name, test_def->name, sizeof(result->name));
    safe_strcpy(result->suite, test_def->suite, sizeof(result->suite));
    result->type = test_def->type;
    result->status = TEST_STATUS_NOT_RUN;
    result->priority = test_def->priority;
    result->start_time = get_current_time_ms();

    /* Инициализация массива ассертов */
    result->assertions_capacity = 100;
    result->assertions = calloc(result->assertions_capacity, sizeof(test_assertion_t));
    if (!result->assertions) {
        free(result);
        return NULL;
    }

    return result;
}

/**
 * @brief Уничтожение результата теста
 */
static void destroy_test_result(test_result_t *result) {
    if (result) {
        if (result->assertions) {
            free(result->assertions);
        }
        free(result);
    }
}

/**
 * @brief Выполнение отдельного теста
 */
static int run_single_test(test_definition_t *test_def, test_result_t *result) {
    pthread_mutex_lock(&framework_mutex);

    /* Обновляем статистику */
    global_statistics.total_tests++;

    result->status = TEST_STATUS_RUNNING;
    result->start_time = get_current_time_ms();

    pthread_mutex_unlock(&framework_mutex);

    /* Выполнение настройки теста */
    if (test_def->setup_func) {
        test_def->setup_func(&global_context);
    }

    /* Выполнение самого теста */
    if (test_def->test_func) {
        test_def->test_func(result, &global_context);
    }

    /* Выполнение очистки теста */
    if (test_def->teardown_func) {
        test_def->teardown_func(&global_context);
    }

    /* Обновление времени завершения */
    result->end_time = get_current_time_ms();
    result->duration_ms = result->end_time - result->start_time;

    /* Обновление статистики */
    pthread_mutex_lock(&framework_mutex);

    global_statistics.total_duration_ms += result->duration_ms;
    global_statistics.total_assertions += result->assertion_count;

    if (result->status == TEST_STATUS_PASSED) {
        global_statistics.tests_passed++;
        global_statistics.passed_assertions += result->passed_assertions;
    } else if (result->status == TEST_STATUS_FAILED) {
        global_statistics.tests_failed++;
        global_statistics.failed_assertions += result->failed_assertions;
    }

    pthread_mutex_unlock(&framework_mutex);

    return 0;
}

/**
 * @brief Регистрация обработчика событий
 */
static int register_event_handler_internal(test_event_type_t event_type,
                                          void (*callback)(test_event_type_t, void *, void *),
                                          void *user_data) {
    event_handler_t *handler = malloc(sizeof(event_handler_t));
    if (!handler) {
        return -1;
    }

    handler->event_type = event_type;
    handler->callback = callback;
    handler->user_data = user_data;
    handler->next = event_handlers;

    event_handlers = handler;
    return 0;
}

/**
 * @brief Отправка события всем обработчикам
 */
static void emit_event_internal(test_event_type_t event_type, void *event_data) {
    pthread_mutex_lock(&event_mutex);

    event_handler_t *handler = event_handlers;
    while (handler) {
        if (handler->event_type == event_type && handler->callback) {
            handler->callback(event_type, event_data, handler->user_data);
        }
        handler = handler->next;
    }

    pthread_mutex_unlock(&event_mutex);
}

/* ============================================================================
 * Реализация API функций
 * ============================================================================ */

int test_framework_init(const test_config_t *config) {
    if (framework_initialized) {
        return -1;
    }

    pthread_mutex_lock(&framework_mutex);

    /* Инициализация глобального контекста */
    memset(&global_context, 0, sizeof(test_context_t));
    safe_strcpy(global_context.app_name, "SystemMonitor Test Framework",
                sizeof(global_context.app_name));
    safe_strcpy(global_context.app_version, "3.0.0",
                sizeof(global_context.app_version));

    /* Копирование конфигурации */
    if (config) {
        memcpy(&global_context.config, config, sizeof(test_config_t));
    } else {
        /* Значения по умолчанию */
        global_context.config.verbose = false;
        global_context.config.colored_output = true;
        global_context.config.xml_output = false;
        global_context.config.json_output = false;
        global_context.config.default_timeout_ms = 5000;
        global_context.config.max_concurrent_tests = 4;
        safe_strcpy(global_context.config.output_dir, "./test-results",
                    sizeof(global_context.config.output_dir));
    }

    /* Инициализация статистики */
    memset(&global_statistics, 0, sizeof(test_statistics_t));

    /* Создание директории для результатов */
    if (mkdir(global_context.config.output_dir, 0755) != 0 && errno != EEXIST) {
        pthread_mutex_unlock(&framework_mutex);
        return -1;
    }

    global_context.start_time = get_current_time_ms();
    global_context.initialized = true;
    framework_initialized = true;

    pthread_mutex_unlock(&framework_mutex);

    /* Отправка события инициализации */
    emit_event_internal(TEST_EVENT_FRAMEWORK_READY, NULL);

    return 0;
}

int test_framework_cleanup(void) {
    if (!framework_initialized) {
        return -1;
    }

    pthread_mutex_lock(&framework_mutex);

    /* Отправка события завершения */
    emit_event_internal(TEST_EVENT_FRAMEWORK_SHUTDOWN, NULL);

    /* Очистка групп тестов */
    for (size_t i = 0; i < suite_count; ++i) {
        if (test_suites[i]) {
            test_suite_destroy(test_suites[i]);
        }
    }

    /* Очистка обработчиков событий */
    event_handler_t *handler = event_handlers;
    while (handler) {
        event_handler_t *next = handler->next;
        free(handler);
        handler = next;
    }
    event_handlers = NULL;

    /* Очистка глобальных переменных */
    memset(&global_context, 0, sizeof(test_context_t));
    memset(&global_statistics, 0, sizeof(test_statistics_t));
    suite_count = 0;

    framework_initialized = false;

    pthread_mutex_unlock(&framework_mutex);

    return 0;
}

int test_framework_run_all(void) {
    if (!framework_initialized) {
        return -1;
    }

    pthread_mutex_lock(&framework_mutex);

    /* Запуск всех групп тестов */
    for (size_t i = 0; i < suite_count; ++i) {
        if (test_suites[i]) {
            test_suite_run(test_suites[i]);
        }
    }

    pthread_mutex_unlock(&framework_mutex);

    return 0;
}

int test_framework_run_filtered(const char *filter __attribute__((unused))) {
    if (!framework_initialized) {
        return -1;
    }

    /* Заглушка для фильтрации - в полной реализации будет поиск по паттерну */
    return test_framework_run_all();
}

int test_framework_get_statistics(test_statistics_t *stats) {
    if (!stats || !framework_initialized) {
        return -1;
    }

    pthread_mutex_lock(&framework_mutex);
    memcpy(stats, &global_statistics, sizeof(test_statistics_t));

    /* Вычисление производных метрик */
    if (global_statistics.total_tests > 0) {
        stats->success_rate = (double)global_statistics.tests_passed /
                              global_statistics.total_tests * 100.0;
        stats->avg_test_duration_ms = global_statistics.total_duration_ms /
                                      global_statistics.total_tests;
    }

    pthread_mutex_unlock(&framework_mutex);

    return 0;
}

int test_framework_generate_report(const char *format, char *output, size_t max_size) {
    if (!output || max_size == 0) {
        return -1;
    }

    test_statistics_t stats;
    test_framework_get_statistics(&stats);

    if (strcmp(format, "json") == 0) {
        snprintf(output, max_size,
                 "{\n"
                 "  \"framework_version\": \"3.0.0\",\n"
                 "  \"total_tests\": %u,\n"
                 "  \"tests_passed\": %u,\n"
                 "  \"tests_failed\": %u,\n"
                 "  \"success_rate\": %.2f,\n"
                 "  \"total_assertions\": %u,\n"
                 "  \"passed_assertions\": %u,\n"
                 "  \"failed_assertions\": %u\n"
                 "}",
                 stats.total_tests, stats.tests_passed, stats.tests_failed,
                 stats.success_rate, stats.total_assertions,
                 stats.passed_assertions, stats.failed_assertions);
    } else if (strcmp(format, "xml") == 0) {
        snprintf(output, max_size,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<test-report>\n"
                 "  <framework-version>3.0.0</framework-version>\n"
                 "  <summary>\n"
                 "    <total-tests>%u</total-tests>\n"
                 "    <tests-passed>%u</tests-passed>\n"
                 "    <tests-failed>%u</tests-failed>\n"
                 "    <success-rate>%.2f</success-rate>\n"
                 "    <total-assertions>%u</total-assertions>\n"
                 "  </summary>\n"
                 "</test-report>",
                 stats.total_tests, stats.tests_passed, stats.tests_failed,
                 stats.success_rate, stats.total_assertions);
    } else {
        /* Формат по умолчанию - текст */
        snprintf(output, max_size,
                 "SystemMonitor Test Framework v3.0.0\n"
                 "=====================================\n"
                 "Total tests: %u\n"
                 "Passed: %u\n"
                 "Failed: %u\n"
                 "Success rate: %.2f%%\n"
                 "Total assertions: %u\n"
                 "Duration: %llu ms\n",
                 stats.total_tests, stats.tests_passed, stats.tests_failed,
                 stats.success_rate, stats.total_assertions,
                 (unsigned long long)stats.total_duration_ms);
    }

    return 0;
}

test_suite_t *test_suite_create(const char *name, const char *description, test_type_t type) {
    if (!name) {
        return NULL;
    }

    test_suite_t *suite = calloc(1, sizeof(test_suite_t));
    if (!suite) {
        return NULL;
    }

    safe_strcpy(suite->name, name, sizeof(suite->name));
    if (description) {
        safe_strcpy(suite->description, description, sizeof(suite->description));
    }
    suite->type = type;

    /* Инициализация массива тестов */
    suite->test_capacity = 50;
    suite->tests = calloc(suite->test_capacity, sizeof(test_definition_t));
    if (!suite->tests) {
        free(suite);
        return NULL;
    }

    /* Добавление в глобальный массив */
    if (add_suite_to_array(suite) != 0) {
        free(suite->tests);
        free(suite);
        return NULL;
    }

    return suite;
}

int test_suite_destroy(test_suite_t *suite) {
    if (!suite) {
        return -1;
    }

    if (suite->tests) {
        free(suite->tests);
    }

    free(suite);
    return 0;
}

int test_suite_add_test(test_suite_t *suite, const test_definition_t *test) {
    if (!suite || !test) {
        return -1;
    }

    pthread_mutex_lock(&framework_mutex);

    if (suite->test_count >= suite->test_capacity) {
        /* Расширение массива */
        size_t new_capacity = suite->test_capacity * 2;
        test_definition_t *new_tests = realloc(suite->tests,
                                              new_capacity * sizeof(test_definition_t));
        if (!new_tests) {
            pthread_mutex_unlock(&framework_mutex);
            return -1;
        }
        suite->tests = new_tests;
        suite->test_capacity = new_capacity;
    }

    memcpy(&suite->tests[suite->test_count], test, sizeof(test_definition_t));
    suite->test_count++;

    pthread_mutex_unlock(&framework_mutex);

    return 0;
}

int test_suite_run(test_suite_t *suite) {
    if (!suite) {
        return -1;
    }

    /* Отправка события начала группы */
    emit_event_internal(TEST_EVENT_SUITE_STARTED, suite->name);

    /* Выполнение настройки группы */
    if (suite->suite_setup) {
        suite->suite_setup(&global_context);
    }

    /* Выполнение всех тестов в группе */
    for (size_t i = 0; i < suite->test_count; ++i) {
        test_definition_t *test_def = &suite->tests[i];

        if (!test_def->enabled) {
            continue;
        }

        test_result_t *result = create_test_result(test_def);
        if (!result) {
            continue;
        }

        run_single_test(test_def, result);

        /* Освобождение ресурсов результата */
        destroy_test_result(result);
    }

    /* Выполнение очистки группы */
    if (suite->suite_teardown) {
        suite->suite_teardown(&global_context);
    }

    /* Отправка события завершения группы */
    emit_event_internal(TEST_EVENT_SUITE_COMPLETED, suite->name);

    return 0;
}

int test_register(const test_definition_t *test_definition) {
    if (!test_definition || !framework_initialized) {
        return -1;
    }

    /* Поиск группы тестов */
    test_suite_t *suite = find_suite_by_name(test_definition->suite);
    if (!suite) {
        /* Создание новой группы если не найдена */
        suite = test_suite_create(test_definition->suite, NULL, test_definition->type);
        if (!suite) {
            return -1;
        }
    }

    /* Добавление теста в группу */
    return test_suite_add_test(suite, test_definition);
}

test_definition_t *test_definition_create(const char *name,
                                        const char *suite_name,
                                        test_function_t test_func,
                                        test_type_t type) {
    if (!name || !suite_name || !test_func) {
        return NULL;
    }

    test_definition_t *test_def = malloc(sizeof(test_definition_t));
    if (!test_def) {
        return NULL;
    }

    memset(test_def, 0, sizeof(test_definition_t));

    safe_strcpy(test_def->name, name, sizeof(test_def->name));
    safe_strcpy(test_def->suite, suite_name, sizeof(test_def->suite));
    safe_strcpy(test_def->file, "unknown", sizeof(test_def->file));

    test_def->type = type;
    test_def->priority = TEST_PRIORITY_NORMAL;
    test_def->timeout_ms = 5000;
    test_def->test_func = test_func;
    test_def->enabled = true;

    return test_def;
}

int test_definition_destroy(test_definition_t *test_def) {
    if (test_def) {
        if (test_def->tags) {
            free(test_def->tags);
        }
        if (test_def->requirements) {
            free(test_def->requirements);
        }
        free(test_def);
    }
    return 0;
}

test_context_t *test_get_context(void) {
    if (!framework_initialized) {
        return NULL;
    }
    return &global_context;
}

int test_context_set_data(const char *key __attribute__((unused)), void *data __attribute__((unused))) {
    /* Заглушка - в полной реализации будет хранение данных */
    return 0;
}

void *test_context_get_data(const char *key __attribute__((unused))) {
    /* Заглушка - в полной реализации будет получение данных */
    return NULL;
}

int test_get_metrics(const char *test_name __attribute__((unused)), test_metrics_t *metrics) {
    if (!metrics) {
        return -1;
    }

    /* Заглушка - в полной реализации будет сбор реальных метрик */
    memset(metrics, 0, sizeof(test_metrics_t));
    metrics->cpu_usage_percent = 0.0;
    metrics->memory_used_bytes = 0;
    metrics->execution_time_ms = 0;

    return 0;
}

int test_enable_performance_monitoring(bool enable __attribute__((unused))) {
    /* Заглушка - в полной реализации будет включение мониторинга */
    return 0;
}

int test_register_event_handler(test_event_type_t event_type,
                               void (*callback)(test_event_type_t, void *, void *),
                               void *user_data) {
    return register_event_handler_internal(event_type, callback, user_data);
}

int test_emit_event(test_event_type_t event_type, void *event_data) {
    if (!framework_initialized) {
        return -1;
    }

    emit_event_internal(event_type, event_data);
    return 0;
}

/* ============================================================================
 * Утилитарные функции
 * ============================================================================ */

void test_generate_unique_name(const char *base_name, char *buffer, size_t buffer_size) {
    if (!base_name || !buffer || buffer_size == 0) {
        return;
    }

    uint64_t timestamp = get_current_time_ms();
    snprintf(buffer, buffer_size, "%s_%llu", base_name, (unsigned long long)timestamp);
}

void test_framework_version(char *buffer, size_t buffer_size) {
    if (buffer && buffer_size > 0) {
        snprintf(buffer, buffer_size, "SystemMonitor Test Framework v%d.%d.%d",
                 TEST_FRAMEWORK_VERSION_MAJOR,
                 TEST_FRAMEWORK_VERSION_MINOR,
                 TEST_FRAMEWORK_VERSION_PATCH);
    }
}

void test_set_colored_output(bool enable) {
    if (framework_initialized) {
        global_context.config.colored_output = enable;
    }
}

const char *test_get_system_info(void) {
    static char info[256];
    snprintf(info, sizeof(info),
             "Platform: %s, Framework: v%d.%d.%d, Tests: %u",
             "Unknown", TEST_FRAMEWORK_VERSION_MAJOR,
             TEST_FRAMEWORK_VERSION_MINOR, TEST_FRAMEWORK_VERSION_PATCH,
             global_statistics.total_tests);
    return info;
}

/* ============================================================================
 * Глобальные переменные для внешнего доступа
 * ============================================================================ */

test_context_t *global_test_context = &global_context;
test_statistics_t *global_test_stats = &global_statistics;