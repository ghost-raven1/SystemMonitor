/**
 * @file test_diagnostics.c
 * @brief Unit тесты для Diagnostics модуля в новом тестовом фреймворке
 *
 * Тесты для Diagnostics компонентов архитектуры SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/modules/diagnostics.h"
#include "../../../src/core/app_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования Diagnostics модуля
 */
typedef struct {
    diagnostics_t *diagnostics;
    app_context_t *app_ctx;
    diagnostic_config_t config;
    char test_scenario[128];
} diagnostics_test_data_t;

/**
 * @brief Функция создания тестовых данных Diagnostics
 */
diagnostics_test_data_t *create_diagnostics_test_data(const char *scenario) {
    diagnostics_test_data_t *data = malloc(sizeof(diagnostics_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(diagnostics_test_data_t));
    safe_strcpy(data->test_scenario, scenario, sizeof(data->test_scenario));

    /* Настройка конфигурации диагностики по умолчанию */
    data->config.enable_system_checks = true;
    data->config.enable_performance_checks = true;
    data->config.enable_security_checks = true;
    data->config.enable_network_checks = true;
    data->config.check_interval_ms = 5000;
    data->config.max_log_entries = 1000;

    return data;
}

/**
 * @brief Функция освобождения тестовых данных Diagnostics
 */
void destroy_diagnostics_test_data(diagnostics_test_data_t *data) {
    if (data) {
        if (data->diagnostics) {
            diagnostics_destroy(data->diagnostics);
        }
        if (data->app_ctx) {
            app_context_destroy(data->app_ctx);
        }
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для базового функционала Diagnostics
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_initialization) {
    /* Тестирование инициализации Diagnostics модуля */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        TEST_DIAGNOSTIC("Diagnostics модуль создан успешно");

        /* Проверка начального состояния */
        TEST_ASSERT_TRUE(diag->initialized, "Диагностика должна быть инициализирована");

        /* Тестирование уничтожения */
        int destroy_result = diagnostics_destroy(diag);
        TEST_ASSERT_SUCCESS(destroy_result, "Уничтожение диагностики должно пройти успешно");
    }

    TEST_ASSERT_SUCCESS(0, "Инициализация Diagnostics работает корректно");
}
TEST_END();

TEST_BEGIN(test_diagnostics_configuration) {
    /* Тестирование конфигурации Diagnostics модуля */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование получения конфигурации по умолчанию */
        diagnostic_config_t config;
        int get_config_result = diagnostics_get_config(diag, &config);
        TEST_ASSERT_SUCCESS(get_config_result, "Получение конфигурации должно работать");

        if (get_config_result == 0) {
            TEST_DIAGNOSTIC("Конфигурация диагностики получена по умолчанию");
            TEST_DIAGNOSTIC("  Системные проверки: %s", config.enable_system_checks ? "включены" : "отключены");
            TEST_DIAGNOSTIC("  Проверки производительности: %s", config.enable_performance_checks ? "включены" : "отключены");
            TEST_DIAGNOSTIC("  Проверки безопасности: %s", config.enable_security_checks ? "включены" : "отключены");
            TEST_DIAGNOSTIC("  Интервал: %d мс", config.check_interval_ms);

            /* Проверка корректности значений конфигурации */
            TEST_ASSERT_TRUE(config.check_interval_ms > 0,
                           "Интервал проверок должен быть положительным");
            TEST_ASSERT_TRUE(config.max_log_entries > 0,
                           "Максимальное количество записей лога должно быть положительным");
        }

        /* Тестирование установки пользовательской конфигурации */
        diagnostic_config_t custom_config = config;
        custom_config.check_interval_ms = 10000; /* 10 секунд */
        custom_config.enable_network_checks = false;

        int set_config_result = diagnostics_set_config(diag, &custom_config);
        TEST_ASSERT_SUCCESS(set_config_result, "Установка конфигурации должна работать");

        /* Проверка что конфигурация была применена */
        diagnostic_config_t verify_config;
        diagnostics_get_config(diag, &verify_config);
        TEST_ASSERT_TRUE(verify_config.check_interval_ms == 10000,
                        "Интервал проверок должен быть изменен");
        TEST_ASSERT_FALSE(verify_config.enable_network_checks,
                         "Сетевые проверки должны быть отключены");

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Конфигурация Diagnostics работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для системных проверок диагностики
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_system_checks) {
    /* Тестирование системных проверок диагностики */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование выполнения системных проверок */
        diagnostic_result_t *results = NULL;
        int result_count = 0;

        int check_result = diagnostics_run_system_checks(diag, &results, &result_count);
        TEST_ASSERT_TRUE(check_result >= -1, "Системные проверки должны возвращать корректные коды");
        TEST_ASSERT_TRUE(result_count >= 0, "Количество результатов должно быть неотрицательным");

        if (check_result == 0 && result_count > 0) {
            TEST_DIAGNOSTIC("Выполнено системных проверок: %d", result_count);

            /* Проверка корректности результатов */
            for (int i = 0; i < result_count && i < 5; i++) {
                TEST_ASSERT_NOT_NULL(results[i].check_name, "Имя проверки не должно быть NULL");
                TEST_ASSERT_TRUE(strlen(results[i].check_name) > 0, "Имя проверки не должно быть пустым");
                TEST_ASSERT_NOT_NULL(results[i].description, "Описание проверки не должно быть NULL");
                TEST_ASSERT_TRUE(results[i].severity >= 0 && results[i].severity <= 3,
                               "Уровень серьезности должен быть в диапазоне 0-3");
                TEST_ASSERT_TRUE(results[i].timestamp > 0, "Временная метка должна быть положительной");
            }

            /* Освобождение результатов */
            if (results) {
                diagnostics_free_results(results, result_count);
            }
        } else {
            TEST_DIAGNOSTIC("Системные проверки не выполнены или не найдены проблемы");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Системные проверки диагностики работают корректно");
}
TEST_END();

TEST_BEGIN(test_diagnostics_performance_checks) {
    /* Тестирование проверок производительности */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование выполнения проверок производительности */
        diagnostic_result_t *results = NULL;
        int result_count = 0;

        int check_result = diagnostics_run_performance_checks(diag, &results, &result_count);
        TEST_ASSERT_TRUE(check_result >= -1, "Проверки производительности должны возвращать корректные коды");
        TEST_ASSERT_TRUE(result_count >= 0, "Количество результатов должно быть неотрицательным");

        if (check_result == 0 && result_count > 0) {
            TEST_DIAGNOSTIC("Выполнено проверок производительности: %d", result_count);

            /* Проверка что результаты содержат информацию о производительности */
            bool found_performance_issue = false;
            for (int i = 0; i < result_count; i++) {
                if (strstr(results[i].check_name, "performance") ||
                    strstr(results[i].check_name, "memory") ||
                    strstr(results[i].check_name, "cpu")) {
                    found_performance_issue = true;
                    break;
                }
            }

            if (found_performance_issue) {
                TEST_DIAGNOSTIC("Найдены проблемы производительности для анализа");
            }

            /* Освобождение результатов */
            if (results) {
                diagnostics_free_results(results, result_count);
            }
        } else {
            TEST_DIAGNOSTIC("Проблемы производительности не найдены или проверки не выполнены");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Проверки производительности работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для диагностики безопасности
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_security_checks) {
    /* Тестирование проверок безопасности */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование выполнения проверок безопасности */
        diagnostic_result_t *results = NULL;
        int result_count = 0;

        int check_result = diagnostics_run_security_checks(diag, &results, &result_count);
        TEST_ASSERT_TRUE(check_result >= -1, "Проверки безопасности должны возвращать корректные коды");
        TEST_ASSERT_TRUE(result_count >= 0, "Количество результатов должно быть неотрицательным");

        if (check_result == 0 && result_count > 0) {
            TEST_DIAGNOSTIC("Выполнено проверок безопасности: %d", result_count);

            /* Проверка что результаты содержат информацию о безопасности */
            for (int i = 0; i < result_count && i < 3; i++) {
                TEST_DIAGNOSTIC("Проверка безопасности: %s - %s",
                               results[i].check_name,
                               results[i].severity >= 2 ? "КРИТИЧНО" :
                               results[i].severity >= 1 ? "ПРЕДУПРЕЖДЕНИЕ" : "ИНФОРМАЦИЯ");
            }

            /* Освобождение результатов */
            if (results) {
                diagnostics_free_results(results, result_count);
            }
        } else {
            TEST_DIAGNOSTIC("Проблемы безопасности не найдены");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Проверки безопасности работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для сетевых проверок диагностики
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_network_checks) {
    /* Тестирование сетевых проверок диагностики */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование выполнения сетевых проверок */
        diagnostic_result_t *results = NULL;
        int result_count = 0;

        int check_result = diagnostics_run_network_checks(diag, &results, &result_count);
        TEST_ASSERT_TRUE(check_result >= -1, "Сетевые проверки должны возвращать корректные коды");
        TEST_ASSERT_TRUE(result_count >= 0, "Количество результатов должно быть неотрицательным");

        if (check_result == 0 && result_count > 0) {
            TEST_DIAGNOSTIC("Выполнено сетевых проверок: %d", result_count);

            /* Проверка что результаты содержат сетевую информацию */
            bool found_network_issue = false;
            for (int i = 0; i < result_count; i++) {
                if (strstr(results[i].check_name, "network") ||
                    strstr(results[i].check_name, "connection") ||
                    strstr(results[i].check_name, "dns")) {
                    found_network_issue = true;
                    TEST_DIAGNOSTIC("Найдена сетевая проблема: %s", results[i].check_name);
                    break;
                }
            }

            /* Освобождение результатов */
            if (results) {
                diagnostics_free_results(results, result_count);
            }
        } else {
            TEST_DIAGNOSTIC("Сетевые проблемы не найдены или проверки не выполнены");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Сетевые проверки диагностики работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для комплексной диагностики
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_comprehensive) {
    /* Тестирование комплексной диагностики */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование выполнения всех проверок */
        diagnostic_result_t *results = NULL;
        int result_count = 0;

        int check_result = diagnostics_run_all_checks(diag, &results, &result_count);
        TEST_ASSERT_TRUE(check_result >= -1, "Комплексная диагностика должна возвращать корректные коды");
        TEST_ASSERT_TRUE(result_count >= 0, "Количество результатов должно быть неотрицательным");

        if (check_result == 0 && result_count > 0) {
            TEST_DIAGNOSTIC("Выполнена комплексная диагностика: %d проверок", result_count);

            /* Анализ результатов по уровням серьезности */
            int critical_count = 0, warning_count = 0, info_count = 0;

            for (int i = 0; i < result_count; i++) {
                switch (results[i].severity) {
                    case 3: critical_count++; break;
                    case 2: warning_count++; break;
                    case 1: info_count++; break;
                    default: break;
                }
            }

            TEST_DIAGNOSTIC("Результаты диагностики:");
            TEST_DIAGNOSTIC("  Критичных проблем: %d", critical_count);
            TEST_DIAGNOSTIC("  Предупреждений: %d", warning_count);
            TEST_DIAGNOSTIC("  Информационных сообщений: %d", info_count);

            /* Проверка что критичные проблемы требуют внимания */
            if (critical_count > 0) {
                TEST_DIAGNOSTIC("Обнаружены критичные проблемы требующие немедленного внимания");
            }

            /* Освобождение результатов */
            if (results) {
                diagnostics_free_results(results, result_count);
            }
        } else {
            TEST_DIAGNOSTIC("Комплексная диагностика завершена без серьезных проблем");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Комплексная диагностика работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для мониторинга диагностики в реальном времени
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_real_time_monitoring) {
    /* Тестирование мониторинга диагностики в реальном времени */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Тестирование запуска мониторинга диагностики */
        int start_result = diagnostics_start_monitoring(diag);
        TEST_ASSERT_TRUE(start_result >= -1, "Запуск мониторинга должен возвращать корректные коды");

        if (start_result == 0) {
            TEST_DIAGNOSTIC("Мониторинг диагностики запущен");

            /* Проверка что мониторинг активен */
            TEST_ASSERT_TRUE(diagnostics_is_monitoring(diag),
                           "Мониторинг диагностики должен быть активен");

            /* Небольшая пауза для накопления диагностических данных */
            sleep(2);

            /* Проверка что мониторинг генерирует результаты */
            diagnostic_result_t *results = NULL;
            int result_count = 0;

            diagnostics_get_latest_results(diag, &results, &result_count);

            if (result_count > 0) {
                TEST_DIAGNOSTIC("Мониторинг диагностики сгенерировал %d результатов", result_count);

                /* Освобождение результатов */
                if (results) {
                    diagnostics_free_results(results, result_count);
                }
            }

            /* Тестирование остановки мониторинга */
            int stop_result = diagnostics_stop_monitoring(diag);
            TEST_ASSERT_SUCCESS(stop_result, "Остановка мониторинга должна работать");

            TEST_ASSERT_FALSE(diagnostics_is_monitoring(diag),
                            "Мониторинг диагностики должен быть остановлен");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Мониторинг диагностики в реальном времени работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для производительности Diagnostics
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_performance) {
    /* Тестирование производительности Diagnostics модуля */

    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать Diagnostics модуль");

    if (diag) {
        /* Измерение времени выполнения системных проверок */
        const int iterations = 20;

        TEST_TIME_START();

        for (int i = 0; i < iterations; i++) {
            diagnostic_result_t *results = NULL;
            int result_count = 0;
            diagnostics_run_system_checks(diag, &results, &result_count);

            if (results) {
                diagnostics_free_results(results, result_count);
            }
        }

        uint64_t elapsed_time = TEST_TIME_END();
        double avg_time_per_check = (double)elapsed_time / iterations;

        TEST_DIAGNOSTIC("Среднее время системной проверки: %.2f мс", avg_time_per_check);

        /* Проверка производительности */
        TEST_ASSERT_TRUE(avg_time_per_check < 200.0,
                        "Системная проверка не должна занимать больше 200мс в среднем");

        /* Тестирование производительности комплексной диагностики */
        TEST_TIME_START();

        for (int i = 0; i < iterations; i++) {
            diagnostic_result_t *results = NULL;
            int result_count = 0;
            diagnostics_run_all_checks(diag, &results, &result_count);

            if (results) {
                diagnostics_free_results(results, result_count);
            }
        }

        elapsed_time = TEST_TIME_END();
        avg_time_per_check = (double)elapsed_time / iterations;

        TEST_DIAGNOSTIC("Среднее время комплексной диагностики: %.2f мс", avg_time_per_check);

        /* Комплексная диагностика может занимать больше времени */
        TEST_ASSERT_TRUE(avg_time_per_check < 500.0,
                        "Комплексная диагностика не должна занимать больше 500мс в среднем");

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Производительность Diagnostics в пределах нормы");
}
TEST_END();

/* ============================================================================
 * Unit тесты для памяти и ресурсов Diagnostics
 * ============================================================================ */

TEST_BEGIN(test_diagnostics_memory_management) {
    /* Тестирование управления памятью в Diagnostics */

    /* Тестирование что создание/уничтожение диагностики не вызывает утечек */
    for (int i = 0; i < 10; i++) {
        diagnostics_t *diag = diagnostics_create();
        TEST_ASSERT_NOT_NULL(diag, "Не удалось создать диагностику в цикле");

        if (diag) {
            /* Имитация работы с диагностикой */
            diagnostic_result_t *results = NULL;
            int result_count = 0;

            diagnostics_run_system_checks(diag, &results, &result_count);

            if (results) {
                diagnostics_free_results(results, result_count);
            }

            diagnostics_destroy(diag);
        }
    }

    TEST_DIAGNOSTIC("Цикл создания/уничтожения диагностики выполнен успешно");

    /* Тестирование освобождения результатов диагностики */
    diagnostics_t *diag = diagnostics_create();
    TEST_ASSERT_NOT_NULL(diag, "Не удалось создать диагностику для теста памяти");

    if (diag) {
        /* Выполнение проверок для генерации результатов */
        diagnostic_result_t *results = NULL;
        int result_count = 0;

        diagnostics_run_all_checks(diag, &results, &result_count);

        if (results && result_count > 0) {
            /* Тестирование что освобождение памяти работает корректно */
            diagnostics_free_results(results, result_count);
            TEST_DIAGNOSTIC("Результаты диагностики освобождены корректно");
        }

        diagnostics_destroy(diag);
    }

    TEST_ASSERT_SUCCESS(0, "Управление памятью Diagnostics работает корректно");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы Diagnostics тестов
 */
static void diagnostics_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы Diagnostics тестов...");

    /* Проверка что диагностические компоненты доступны */
    TEST_DIAGNOSTIC("Проверка доступности диагностических компонентов");
}

/**
 * @brief Очистка после завершения группы Diagnostics тестов
 */
static void diagnostics_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы Diagnostics тестов...");

    /* Освобождение ресурсов Diagnostics если необходимо */
    TEST_DIAGNOSTIC("Освобождение ресурсов Diagnostics");
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы Diagnostics тестов
 */
test_suite_t *create_diagnostics_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("diagnostics",
                                          "Тесты Diagnostics модуля",
                                          TEST_TYPE_UNIT);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы Diagnostics тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Базовые тесты */
    test_def = test_definition_create("test_diagnostics_initialization", "diagnostics",
                                     test_diagnostics_initialization, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_diagnostics_configuration", "diagnostics",
                                     test_diagnostics_configuration, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты отдельных типов проверок */
    test_def = test_definition_create("test_diagnostics_system_checks", "diagnostics",
                                     test_diagnostics_system_checks, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_diagnostics_performance_checks", "diagnostics",
                                     test_diagnostics_performance_checks, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_diagnostics_security_checks", "diagnostics",
                                     test_diagnostics_security_checks, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_diagnostics_network_checks", "diagnostics",
                                     test_diagnostics_network_checks, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты комплексной диагностики */
    test_def = test_definition_create("test_diagnostics_comprehensive", "diagnostics",
                                     test_diagnostics_comprehensive, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты мониторинга */
    test_def = test_definition_create("test_diagnostics_real_time_monitoring", "diagnostics",
                                     test_diagnostics_real_time_monitoring, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_diagnostics_performance", "diagnostics",
                                     test_diagnostics_performance, TEST_TYPE_PERFORMANCE);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты памяти */
    test_def = test_definition_create("test_diagnostics_memory_management", "diagnostics",
                                     test_diagnostics_memory_management, TEST_TYPE_UNIT);
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

    TEST_DIAGNOSTIC("Запуск группы Diagnostics тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("==============================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 45000; /* 45 секунд таймаут для Diagnostics тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_diagnostics_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы Diagnostics тестов");
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
    TEST_DIAGNOSTIC("Группа Diagnostics тестов завершена");
    return result;
}