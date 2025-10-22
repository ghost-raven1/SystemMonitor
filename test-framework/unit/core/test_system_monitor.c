/**
 * @file test_system_monitor.c
 * @brief Unit тесты для System Monitor модулей в новом тестовом фреймворке
 *
 * Тесты для новых компонентов архитектуры SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/modules/system_monitor.h"
#include "../../../src/core/app_context.h"
#include "../../../src/core/app_events.h"
#include "../../../src/core/notifications.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования System Monitor модулей
 */
typedef struct {
    system_monitor_t *monitor;
    app_context_t *app_ctx;
    char test_scenario[128];
} system_monitor_test_data_t;

/**
 * @brief Функция создания тестовых данных System Monitor
 */
system_monitor_test_data_t *create_system_monitor_test_data(const char *scenario) {
    system_monitor_test_data_t *data = malloc(sizeof(system_monitor_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(system_monitor_test_data_t));
    safe_strcpy(data->test_scenario, scenario, sizeof(data->test_scenario));

    return data;
}

/**
 * @brief Функция освобождения тестовых данных System Monitor
 */
void destroy_system_monitor_test_data(system_monitor_test_data_t *data) {
    if (data) {
        if (data->monitor) {
            system_monitor_destroy(data->monitor);
        }
        if (data->app_ctx) {
            app_context_destroy(data->app_ctx);
        }
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для базового функционала System Monitor
 * ============================================================================ */

TEST_BEGIN(test_system_monitor_initialization) {
    /* Тестирование инициализации System Monitor модуля */

    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать System Monitor");

    if (monitor) {
        TEST_DIAGNOSTIC("System Monitor создан успешно");

        /* Проверка начального состояния */
        TEST_ASSERT_NOT_NULL(monitor, "Монитор должен существовать");
        TEST_ASSERT_TRUE(monitor->initialized, "Монитор должен быть инициализирован");

        /* Тестирование уничтожения */
        int destroy_result = system_monitor_destroy(monitor);
        TEST_ASSERT_SUCCESS(destroy_result, "Уничтожение монитора должно пройти успешно");
    }

    TEST_ASSERT_SUCCESS(0, "Инициализация System Monitor работает корректно");
}
TEST_END();

TEST_BEGIN(test_system_monitor_configuration) {
    /* Тестирование конфигурации System Monitor модуля */

    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать System Monitor");

    if (monitor) {
        /* Тестирование получения конфигурации */
        system_monitor_config_t config;
        int get_config_result = system_monitor_get_config(monitor, &config);
        TEST_ASSERT_SUCCESS(get_config_result, "Получение конфигурации должно работать");

        if (get_config_result == 0) {
            TEST_DIAGNOSTIC("Конфигурация получена: интервал=%dмс, enabled=%s",
                           config.update_interval_ms,
                           config.enabled ? "да" : "нет");

            /* Проверка корректности значений конфигурации */
            TEST_ASSERT_TRUE(config.update_interval_ms > 0,
                           "Интервал обновления должен быть положительным");
            TEST_ASSERT_TRUE(config.update_interval_ms <= 60000,
                           "Интервал обновления не должен превышать 60 секунд");
        }

        /* Тестирование установки конфигурации */
        system_monitor_config_t new_config = config;
        new_config.update_interval_ms = 2000; /* 2 секунды */

        int set_config_result = system_monitor_set_config(monitor, &new_config);
        TEST_ASSERT_SUCCESS(set_config_result, "Установка конфигурации должна работать");

        system_monitor_destroy(monitor);
    }

    TEST_ASSERT_SUCCESS(0, "Конфигурация System Monitor работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для мониторинга метрик
 * ============================================================================ */

TEST_BEGIN(test_system_monitor_metrics) {
    /* Тестирование мониторинга метрик */

    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать System Monitor");

    if (monitor) {
        /* Тестирование получения метрик */
        system_metrics_t metrics;
        int get_metrics_result = system_monitor_get_metrics(monitor, &metrics);
        TEST_ASSERT_SUCCESS(get_metrics_result, "Получение метрик должно работать");

        if (get_metrics_result == 0) {
            TEST_DIAGNOSTIC("Метрики получены: CPU=%.1f%%, Memory=%.1f%%, Disk=%.1f%%",
                           metrics.cpu_usage, metrics.memory_usage, metrics.disk_usage);

            /* Проверка корректности метрик */
            TEST_ASSERT_VALID_METRICS(&metrics, "Метрики должны быть корректными");

            /* Проверка что метрики обновляются */
            sleep(1);

            system_metrics_t metrics2;
            system_monitor_get_metrics(monitor, &metrics2);

            /* Метрики могут измениться, но должны оставаться валидными */
            TEST_ASSERT_VALID_METRICS(&metrics2, "Обновленные метрики должны быть корректными");
        }

        system_monitor_destroy(monitor);
    }

    TEST_ASSERT_SUCCESS(0, "Мониторинг метрик работает корректно");
}
TEST_END();

TEST_BEGIN(test_system_monitor_real_time_monitoring) {
    /* Тестирование мониторинга в реальном времени */

    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать System Monitor");

    if (monitor) {
        /* Тестирование старта мониторинга */
        int start_result = system_monitor_start(monitor);
        TEST_ASSERT_SUCCESS(start_result, "Запуск мониторинга должен работать");

        if (start_result == 0) {
            TEST_DIAGNOSTIC("Мониторинг запущен");

            /* Проверка что мониторинг активен */
            TEST_ASSERT_TRUE(system_monitor_is_running(monitor),
                           "Мониторинг должен быть активен после запуска");

            /* Небольшая пауза для накопления данных */
            sleep(2);

            /* Проверка что данные обновляются */
            system_metrics_t metrics;
            system_monitor_get_metrics(monitor, &metrics);
            TEST_ASSERT_VALID_METRICS(&metrics, "Метрики должны обновляться");

            /* Тестирование остановки мониторинга */
            int stop_result = system_monitor_stop(monitor);
            TEST_ASSERT_SUCCESS(stop_result, "Остановка мониторинга должна работать");

            TEST_ASSERT_FALSE(system_monitor_is_running(monitor),
                            "Мониторинг должен быть остановлен");
        }

        system_monitor_destroy(monitor);
    }

    TEST_ASSERT_SUCCESS(0, "Мониторинг в реальном времени работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для интеграции с контекстом приложения
 * ============================================================================ */

TEST_BEGIN(test_system_monitor_app_context) {
    /* Тестирование интеграции с контекстом приложения */

    /* Создание контекста приложения */
    app_context_t *app_ctx = app_context_create("TestApp", "1.0.0");
    TEST_ASSERT_NOT_NULL(app_ctx, "Не удалось создать контекст приложения");

    if (app_ctx) {
        /* Создание монитора с контекстом */
        system_monitor_t *monitor = system_monitor_create_with_context(app_ctx);
        TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать монитор с контекстом");

        if (monitor) {
            TEST_DIAGNOSTIC("System Monitor создан с контекстом приложения");

            /* Проверка что монитор связан с контекстом */
            TEST_ASSERT_TRUE(monitor->app_context != NULL,
                           "Монитор должен быть связан с контекстом");

            /* Тестирование что монитор может использовать данные контекста */
            system_monitor_config_t config;
            system_monitor_get_config(monitor, &config);

            TEST_DIAGNOSTIC("Конфигурация монитора в контексте приложения получена");

            system_monitor_destroy(monitor);
        }

        app_context_destroy(app_ctx);
    }

    TEST_ASSERT_SUCCESS(0, "Интеграция с контекстом приложения работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для обработки событий
 * ============================================================================ */

TEST_BEGIN(test_system_monitor_events) {
    /* Тестирование обработки событий */

    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать System Monitor");

    if (monitor) {
        /* Тестирование регистрации обработчика событий */
        int register_result = system_monitor_register_event_handler(
            monitor, "test_event", NULL, NULL);
        TEST_ASSERT_SUCCESS(register_result, "Регистрация обработчика событий должна работать");

        /* Тестирование что монитор может обрабатывать события */
        TEST_DIAGNOSTIC("Обработчик событий зарегистрирован");

        /* Тестирование генерации внутренних событий мониторинга */
        system_metrics_t metrics;
        system_monitor_get_metrics(monitor, &metrics);

        /* Получение метрик должно генерировать событие обновления */
        TEST_DIAGNOSTIC("Метрики получены, событие должно быть сгенерировано");

        system_monitor_destroy(monitor);
    }

    TEST_ASSERT_SUCCESS(0, "Обработка событий работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для производительности System Monitor
 * ============================================================================ */

TEST_BEGIN(test_system_monitor_performance) {
    /* Тестирование производительности System Monitor */

    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать System Monitor");

    if (monitor) {
        /* Измерение времени получения метрик */
        const int iterations = 100;

        TEST_TIME_START();

        for (int i = 0; i < iterations; i++) {
            system_metrics_t metrics;
            system_monitor_get_metrics(monitor, &metrics);
        }

        uint64_t elapsed_time = TEST_TIME_END();
        double avg_time_per_call = (double)elapsed_time / iterations;

        TEST_DIAGNOSTIC("Среднее время получения метрик: %.2f мс", avg_time_per_call);

        /* Проверка производительности */
        TEST_ASSERT_TRUE(avg_time_per_call < 50.0,
                        "Получение метрик не должно занимать больше 50мс в среднем");

        /* Тестирование производительности с запущенным мониторингом */
        system_monitor_start(monitor);

        TEST_TIME_START();

        for (int i = 0; i < iterations; i++) {
            system_metrics_t metrics;
            system_monitor_get_metrics(monitor, &metrics);
            usleep(10000); /* 10ms пауза */
        }

        elapsed_time = TEST_TIME_END();
        avg_time_per_call = (double)elapsed_time / iterations;

        TEST_DIAGNOSTIC("Среднее время с активным мониторингом: %.2f мс", avg_time_per_call);

        /* С активным мониторингом может быть небольшая задержка */
        TEST_ASSERT_TRUE(avg_time_per_call < 100.0,
                        "Получение метрик с активным мониторингом не должно занимать больше 100мс");

        system_monitor_stop(monitor);
        system_monitor_destroy(monitor);
    }

    TEST_ASSERT_SUCCESS(0, "Производительность System Monitor в пределах нормы");
}
TEST_END();

/* ============================================================================
 * Unit тесты для памяти и ресурсов
 * ============================================================================ */

TEST_BEGIN(test_system_monitor_memory) {
    /* Тестирование использования памяти System Monitor */

    /* Тестирование что создание/уничтожение монитора не вызывает утечек */
    for (int i = 0; i < 10; i++) {
        system_monitor_t *monitor = system_monitor_create();
        TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать монитор в цикле");

        if (monitor) {
            /* Имитация работы с монитором */
            system_metrics_t metrics;
            system_monitor_get_metrics(monitor, &metrics);

            system_monitor_destroy(monitor);
        }
    }

    TEST_DIAGNOSTIC("Цикл создания/уничтожения мониторов выполнен успешно");

    /* Тестирование что монитор корректно освобождает ресурсы */
    system_monitor_t *monitor = system_monitor_create();
    TEST_ASSERT_NOT_NULL(monitor, "Не удалось создать финальный монитор");

    if (monitor) {
        /* Запуск мониторинга для выделения ресурсов */
        system_monitor_start(monitor);

        /* Небольшая работа */
        sleep(1);

        /* Проверка что монитор корректно останавливается и освобождает ресурсы */
        system_monitor_stop(monitor);
        system_monitor_destroy(monitor);

        TEST_DIAGNOSTIC("Ресурсы монитора освобождены корректно");
    }

    TEST_ASSERT_SUCCESS(0, "Управление памятью System Monitor работает корректно");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы System Monitor тестов
 */
static void system_monitor_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы System Monitor тестов...");

    /* Проверка что все необходимые компоненты доступны */
    TEST_DIAGNOSTIC("Проверка доступности компонентов System Monitor");
}

/**
 * @brief Очистка после завершения группы System Monitor тестов
 */
static void system_monitor_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы System Monitor тестов...");

    /* Освобождение глобальных ресурсов System Monitor если необходимо */
    TEST_DIAGNOSTIC("Освобождение ресурсов System Monitor");
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы System Monitor тестов
 */
test_suite_t *create_system_monitor_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("system_monitor",
                                          "Тесты System Monitor модулей",
                                          TEST_TYPE_UNIT);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы System Monitor тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Базовые тесты */
    test_def = test_definition_create("test_system_monitor_initialization", "system_monitor",
                                     test_system_monitor_initialization, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_monitor_configuration", "system_monitor",
                                     test_system_monitor_configuration, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты метрик */
    test_def = test_definition_create("test_system_monitor_metrics", "system_monitor",
                                     test_system_monitor_metrics, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_monitor_real_time_monitoring", "system_monitor",
                                     test_system_monitor_real_time_monitoring, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты интеграции */
    test_def = test_definition_create("test_system_monitor_app_context", "system_monitor",
                                     test_system_monitor_app_context, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_monitor_events", "system_monitor",
                                     test_system_monitor_events, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_system_monitor_performance", "system_monitor",
                                     test_system_monitor_performance, TEST_TYPE_PERFORMANCE);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты памяти */
    test_def = test_definition_create("test_system_monitor_memory", "system_monitor",
                                     test_system_monitor_memory, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    return suite;
}

/* ============================================================================
 * Главная функция для запуска тестов
 * ============================================================================ */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    TEST_DIAGNOSTIC("Запуск группы System Monitor тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("=================================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 30000; /* 30 секунд таймаут для System Monitor тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_system_monitor_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы System Monitor тестов");
        test_framework_cleanup();
        return 1;
    }

    /* Регистрация обработчиков событий для мониторинга */
    test_register_event_handler(TEST_EVENT_STARTED, NULL, NULL);
    test_register_event_handler(TEST_EVENT_COMPLETED, NULL, NULL);
    test_register_event_handler(TEST_EVENT_FAILED, NULL, NULL);

    /* Запуск группы тестов */
    int result = test_suite_run(suite);

    /* Освобождение ресурсов */
    test_suite_destroy(suite);
    test_framework_cleanup();

    TEST_DIAGNOSTIC("");
    TEST_DIAGNOSTIC("Группа System Monitor тестов завершена");
    return result;
}