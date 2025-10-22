/**
 * @file test_macros.h
 * @brief Макросы тестирования для удобного создания тестов
 *
 * Этот файл содержит макросы, упрощающие написание тестов в соответствии
 * с архитектурой SystemMonitor. Макросы интегрированы с системой событий
 * и метрик производительности.
 */

#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#include "test_framework.h"
#include "test_asserts.h"

/* ============================================================================
 * Макросы для создания тестовых функций
 * ============================================================================ */

/**
 * @brief Макрос начала теста с автоматической инициализацией
 * @param test_name Имя теста
 */
#define TEST_BEGIN(test_name) \
    void test_name(test_result_t *result, test_context_t *context) { \
        TEST_ASSERT_INIT(result, #test_name); \
        TEST_TIME_START(); \
        TEST_EMIT_EVENT(TEST_EVENT_STARTED, NULL);

/**
 * @brief Макрос завершения теста с автоматической финализацией
 */
#define TEST_END() \
        TEST_TIME_END(); \
        TEST_EMIT_EVENT(TEST_EVENT_COMPLETED, NULL); \
        TEST_ASSERT_FINALIZE(result); \
    }

/**
 * @brief Макрос для создания простого теста
 * @param test_name Имя теста
 * @param suite_name Имя группы тестов
 * @param ... Тело теста
 */
#define SIMPLE_TEST(test_name, suite_name, ...) \
    void test_name(test_result_t *result, test_context_t *context) { \
        TEST_ASSERT_INIT(result, #test_name); \
        TEST_TIME_START(); \
        TEST_EMIT_EVENT(TEST_EVENT_STARTED, NULL); \
        __VA_ARGS__; \
        TEST_TIME_END(); \
        TEST_EMIT_EVENT(TEST_EVENT_COMPLETED, NULL); \
        TEST_ASSERT_FINALIZE(result); \
    }

/* ============================================================================
 * Макросы для работы с ассертами (используют test_asserts.h)
 * ============================================================================ */

/**
 * @brief Проверка равенства значений
 * @param expected Ожидаемое значение
 * @param actual Фактическое значение
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_EQUAL(expected, actual, message) \
    TEST_ASSERT_EQUAL_WITH_MSG(expected, actual, message ? message : "Values are not equal")

/**
 * @brief Проверка равенства строк
 * @param expected Ожидаемая строка
 * @param actual Фактическая строка
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_STRING_EQUAL(expected, actual, message) \
    TEST_ASSERT_STRING_EQUAL_WITH_MSG(expected, actual, message ? message : "Strings are not equal")

/**
 * @brief Проверка условия (true/false)
 * @param condition Условие для проверки
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_TRUE(condition, message) \
    TEST_ASSERT_TRUE_WITH_MSG(condition, message ? message : "Condition is false")

/**
 * @brief Проверка условия (false/true)
 * @param condition Условие для проверки
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_FALSE(condition, message) \
    TEST_ASSERT_FALSE_WITH_MSG(condition, message ? message : "Condition is true")

/**
 * @brief Проверка на NULL
 * @param value Значение для проверки
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_NULL(value, message) \
    TEST_ASSERT_NULL_WITH_MSG(value, message ? message : "Value is not NULL")

/**
 * @brief Проверка на не-NULL
 * @param value Значение для проверки
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_NOT_NULL(value, message) \
    TEST_ASSERT_NOT_NULL_WITH_MSG(value, message ? message : "Value is NULL")

/**
 * @brief Проверка на успешное выполнение функции
 * @param expression Выражение для проверки
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_SUCCESS(expression, message) \
    TEST_ASSERT_SUCCESS_WITH_MSG(expression, message ? message : "Expression failed")

/**
 * @brief Проверка на ошибку выполнения функции
 * @param expression Выражение для проверки
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_ERROR(expression, message) \
    TEST_ASSERT_ERROR_WITH_MSG(expression, message ? message : "Expression should have failed")

/**
 * @brief Проверка принадлежности значения диапазону
 * @param value Значение для проверки
 * @param min Минимальное значение диапазона
 * @param max Максимальное значение диапазона
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_IN_RANGE(value, min, max, message) \
    TEST_ASSERT_IN_RANGE_WITH_MSG(value, min, max, message ? message : "Value is out of range")

/**
 * @brief Проверка размера массива
 * @param array Массив для проверки
 * @param expected_size Ожидаемый размер
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_ARRAY_SIZE(array, expected_size, message) \
    TEST_ASSERT_ARRAY_SIZE_WITH_MSG(array, expected_size, message ? message : "Array size mismatch")

/* ============================================================================
 * Макросы для работы с памятью и ресурсами
 * ============================================================================ */

/**
 * @brief Проверка отсутствия утечек памяти
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_NO_MEMORY_LEAKS(message) \
    do { \
        TEST_MEMORY_CHECKPOINT(); \
        TEST_EMIT_EVENT(TEST_EVENT_MEMORY_WARNING, NULL); \
    } while(0)

/**
 * @brief Проверка времени выполнения (производительность)
 * @param max_time_ms Максимальное допустимое время в миллисекундах
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_PERFORMANCE(max_time_ms, message) \
    do { \
        uint64_t elapsed = TEST_TIME_ELAPSED(); \
        TEST_ASSERT_TRUE(elapsed <= max_time_ms, \
            message ? message : "Performance test failed: execution too slow"); \
        TEST_EMIT_EVENT(TEST_EVENT_PERFORMANCE_WARNING, &elapsed); \
    } while(0)

/* ============================================================================
 * Макросы для работы с модулями SystemMonitor
 * ============================================================================ */

/**
 * @brief Проверка инициализации модуля
 * @param module_name Имя модуля
 * @param context Контекст приложения
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_MODULE_INIT(module_name, context, message) \
    do { \
        module_info_t info; \
        TEST_ASSERT_TRUE(module_get_info(module_name, &info) == 0, \
            message ? message : "Failed to get module info: " module_name); \
        TEST_ASSERT_TRUE(info.status == MODULE_STATUS_RUNNING, \
            message ? message : "Module not running: " module_name); \
    } while(0)

/**
 * @brief Проверка метрик системы
 * @param metrics Структура метрик
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_VALID_METRICS(metrics, message) \
    do { \
        TEST_ASSERT_NOT_NULL(metrics, message ? message : "Metrics is NULL"); \
        TEST_ASSERT_TRUE(metrics->cpu_usage >= 0.0 && metrics->cpu_usage <= 100.0, \
            message ? message : "Invalid CPU usage"); \
        TEST_ASSERT_TRUE(metrics->memory_usage >= 0.0 && metrics->memory_usage <= 100.0, \
            message ? message : "Invalid memory usage"); \
    } while(0)

/* ============================================================================
 * Макросы для работы с плагинами
 * ============================================================================ */

/**
 * @brief Проверка загрузки плагина
 * @param plugin_name Имя плагина
 * @param manager Менеджер плагинов
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_PLUGIN_LOADED(plugin_name, manager, message) \
    do { \
        plugin_info_t info; \
        TEST_ASSERT_TRUE(plugin_get_info(plugin_name, &info) == 0, \
            message ? message : "Plugin not found: " plugin_name); \
        TEST_ASSERT_TRUE(info.enabled, \
            message ? message : "Plugin not enabled: " plugin_name); \
    } while(0)

/* ============================================================================
 * Макросы для работы с событиями
 * ============================================================================ */

/**
 * @brief Проверка получения события
 * @param event_type Тип ожидаемого события
 * @param timeout_ms Таймаут ожидания в миллисекундах
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_EVENT_RECEIVED(event_type, timeout_ms, message) \
    do { \
        test_event_type_t received_event = TEST_WAIT_FOR_EVENT(timeout_ms); \
        TEST_ASSERT_TRUE(received_event == event_type, \
            message ? message : "Expected different event type"); \
    } while(0)

/**
 * @brief Проверка отсутствия событий
 * @param timeout_ms Время ожидания в миллисекундах
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_NO_EVENTS(timeout_ms, message) \
    do { \
        test_event_type_t received_event = TEST_WAIT_FOR_EVENT(timeout_ms); \
        TEST_ASSERT_TRUE(received_event == 0, \
            message ? message : "Unexpected event received"); \
    } while(0)

/* ============================================================================
 * Макросы для работы с конфигурацией
 * ============================================================================ */

/**
 * @brief Проверка значения конфигурации
 * @param config_manager Менеджер конфигурации
 * @param key Ключ конфигурации
 * @param expected_type Ожидаемый тип значения
 * @param message Сообщение об ошибке (опционально)
 */
#define TEST_ASSERT_CONFIG_VALUE(config_manager, key, expected_type, message) \
    do { \
        config_entry_t entry; \
        TEST_ASSERT_TRUE(config_get_value(key, &entry) == 0, \
            message ? message : "Failed to get config value: " key); \
        TEST_ASSERT_TRUE(entry.type == expected_type, \
            message ? message : "Wrong config value type for: " key); \
    } while(0)

/* ============================================================================
 * Макросы для создания фиктивных данных (mocks)
 * ============================================================================ */

/**
 * @brief Создание фиктивного контекста приложения
 */
#define TEST_MOCK_APP_CONTEXT() \
    app_context_t mock_context = { \
        .app_name = "TestApp", \
        .app_version = "1.0.0", \
        .initialized = true \
    };

/**
 * @brief Создание фиктивных метрик системы
 */
#define TEST_MOCK_SYSTEM_METRICS() \
    system_metrics_t mock_metrics = { \
        .cpu_usage = 25.0, \
        .memory_usage = 60.0, \
        .disk_usage = 45.0, \
        .network_rx = 1024, \
        .network_tx = 512, \
        .temperature = 45.0, \
        .battery_level = 80.0, \
        .process_count = 150, \
        .load_average = 1.2, \
        .uptime = 3600 \
    };

/* ============================================================================
 * Макросы для организации тестовых сценариев
 * ============================================================================ */

/**
 * @brief Макрос для создания тестового сценария с настройкой и очисткой
 * @param scenario_name Имя сценария
 * @param setup_code Код настройки
 * @param test_code Код тестирования
 * @param cleanup_code Код очистки
 */
#define TEST_SCENARIO(scenario_name, setup_code, test_code, cleanup_code) \
    void scenario_name(test_result_t *result, test_context_t *context) { \
        TEST_ASSERT_INIT(result, #scenario_name); \
        TEST_TIME_START(); \
        TEST_EMIT_EVENT(TEST_EVENT_STARTED, NULL); \
        \
        /* Настройка */ \
        setup_code; \
        \
        /* Тестирование */ \
        test_code; \
        \
        /* Очистка */ \
        cleanup_code; \
        \
        TEST_TIME_END(); \
        TEST_EMIT_EVENT(TEST_EVENT_COMPLETED, NULL); \
        TEST_ASSERT_FINALIZE(result); \
    }

/**
 * @brief Макрос для параметризованного теста
 * @param test_name Имя теста
 * @param param_type Тип параметра
 * @param params Массив параметров
 * @param params_count Количество параметров
 */
#define PARAMETERIZED_TEST(test_name, param_type, params, params_count) \
    void test_name(test_result_t *result, test_context_t *context) { \
        TEST_ASSERT_INIT(result, #test_name); \
        TEST_TIME_START(); \
        TEST_EMIT_EVENT(TEST_EVENT_STARTED, NULL); \
        \
        for (size_t i = 0; i < params_count; ++i) { \
            param_type param = params[i]; \
            char param_test_name[128]; \
            snprintf(param_test_name, sizeof(param_test_name), "%s_%zu", #test_name, i);

/* ============================================================================
 * Макросы для условного выполнения тестов
 * ============================================================================ */

/**
 * @brief Тест выполняется только на определенной платформе
 * @param platform Платформа (PLATFORM_MACOS, PLATFORM_LINUX, etc.)
 */
#define TEST_ON_PLATFORM(platform) \
    if (get_platform_type() != platform) { \
        TEST_SKIP("Test requires platform: " #platform); \
        return; \
    }

/**
 * @brief Тест выполняется только при наличии определенного модуля
 * @param module_name Имя модуля
 */
#define TEST_REQUIRE_MODULE(module_name) \
    do { \
        module_info_t info; \
        if (module_get_info(module_name, &info) != 0 || !info.enabled) { \
            TEST_SKIP("Required module not available: " module_name); \
            return; \
        } \
    } while(0)

/**
 * @brief Тест требует определенных прав доступа
 * @param permission Право доступа
 */
#define TEST_REQUIRE_PERMISSION(permission) \
    if (!check_permission(permission)) { \
        TEST_SKIP("Required permission not available: " permission); \
        return; \
    }

/* ============================================================================
 * Макросы для диагностики и отладки
 * ============================================================================ */


/**
 * @brief Вывод информации о производительности
 */
#define TEST_PRINT_METRICS() \
    do { \
        test_metrics_t metrics; \
        if (test_get_metrics(result->name, &metrics) == 0) { \
            TEST_DIAGNOSTIC("CPU: %.2f%%, Memory: %llu bytes, Time: %llu ms", \
                metrics.cpu_usage_percent, metrics.memory_used_bytes, metrics.execution_time_ms); \
        } \
    } while(0)

/**
 * @brief Проверка покрытия кода
 * @param function_name Имя функции
 * @param expected_calls Ожидаемое количество вызовов
 */
#define TEST_ASSERT_COVERAGE(function_name, expected_calls) \
    do { \
        int actual_calls = get_function_call_count(function_name); \
        TEST_ASSERT_EQUAL(expected_calls, actual_calls, \
            "Function coverage mismatch for: " function_name); \
    } while(0)

/* ============================================================================
 * Макросы для работы с таймаутами
 * ============================================================================ */

/**
 * @brief Выполнение кода с таймаутом
 * @param timeout_ms Таймаут в миллисекундах
 * @param code Код для выполнения
 */
#define TEST_WITH_TIMEOUT(timeout_ms, code) \
    do { \
        uint64_t start_time = get_current_time_ms(); \
        bool completed = false; \
        \
        /* Здесь должен быть механизм таймаута */ \
        code; \
        completed = true; \
        \
        uint64_t elapsed = get_current_time_ms() - start_time; \
        if (elapsed > timeout_ms) { \
            TEST_FAIL("Operation timed out after " #timeout_ms "ms"); \
        } \
    } while(0)

/**
 * @brief Ожидание события с таймаутом
 * @param event_type Тип ожидаемого события
 * @param timeout_ms Таймаут в миллисекундах
 */
#define TEST_WAIT_FOR_EVENT_WITH_TIMEOUT(event_type, timeout_ms) \
    do { \
        test_event_type_t received_event = 0; \
        uint64_t start_time = get_current_time_ms(); \
        \
        while ((get_current_time_ms() - start_time) < timeout_ms) { \
            received_event = poll_for_event(); \
            if (received_event == event_type) break; \
            sleep_ms(10); \
        } \
        \
        TEST_ASSERT_TRUE(received_event == event_type, \
            "Event " #event_type " not received within timeout"); \
    } while(0)

#endif // TEST_MACROS_H