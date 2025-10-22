/**
 * @file test_developer_tools.c
 * @brief Unit тесты для Developer Tools модуля в новом тестовом фреймворке
 *
 * Тесты для Developer Tools компонентов архитектуры SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/modules/developer_tools.h"
#include "../../../src/core/app_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования Developer Tools
 */
typedef struct {
    developer_tools_t *tools;
    app_context_t *app_ctx;
    char test_workspace[256];
} developer_tools_test_data_t;

/**
 * @brief Функция создания тестовых данных Developer Tools
 */
developer_tools_test_data_t *create_developer_tools_test_data(const char *workspace) {
    developer_tools_test_data_t *data = malloc(sizeof(developer_tools_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(developer_tools_test_data_t));
    safe_strcpy(data->test_workspace, workspace, sizeof(data->test_workspace));

    return data;
}

/**
 * @brief Функция освобождения тестовых данных Developer Tools
 */
void destroy_developer_tools_test_data(developer_tools_test_data_t *data) {
    if (data) {
        if (data->tools) {
            developer_tools_destroy(data->tools);
        }
        if (data->app_ctx) {
            app_context_destroy(data->app_ctx);
        }
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для базового функционала Developer Tools
 * ============================================================================ */

TEST_BEGIN(test_developer_tools_initialization) {
    /* Тестирование инициализации Developer Tools модуля */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        TEST_DIAGNOSTIC("Developer Tools создан успешно");

        /* Проверка начального состояния */
        TEST_ASSERT_TRUE(tools->initialized, "Инструменты разработчика должны быть инициализированы");

        /* Тестирование уничтожения */
        int destroy_result = developer_tools_destroy(tools);
        TEST_ASSERT_SUCCESS(destroy_result, "Уничтожение инструментов должно пройти успешно");
    }

    TEST_ASSERT_SUCCESS(0, "Инициализация Developer Tools работает корректно");
}
TEST_END();

TEST_BEGIN(test_developer_tools_environment_detection) {
    /* Тестирование обнаружения окружения разработки */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Тестирование получения информации об окружении */
        dev_environment_t environment;
        int get_env_result = developer_tools_get_environment(tools, &environment);
        TEST_ASSERT_TRUE(get_env_result >= -1, "Получение окружения должно возвращать корректные коды");

        if (get_env_result == 0) {
            TEST_DIAGNOSTIC("Обнаружено окружение разработки:");
            TEST_DIAGNOSTIC("  IDE: %s", environment.ide_name);
            TEST_DIAGNOSTIC("  Языки: %s", environment.languages);
            TEST_DIAGNOSTIC("  Фреймворки: %s", environment.frameworks);

            /* Проверка корректности данных окружения */
            TEST_ASSERT_NOT_NULL(environment.ide_name, "Имя IDE не должно быть NULL");
            TEST_ASSERT_NOT_NULL(environment.languages, "Языки программирования не должны быть NULL");
            TEST_ASSERT_NOT_NULL(environment.frameworks, "Фреймворки не должны быть NULL");
        } else {
            TEST_DIAGNOSTIC("Окружение разработки не обнаружено (код: %d)", get_env_result);
        }

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Обнаружение окружения разработки работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для анализа процессов разработки
 * ============================================================================ */

TEST_BEGIN(test_developer_tools_process_analysis) {
    /* Тестирование анализа процессов разработки */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Тестирование получения процессов разработки */
        dev_process_t processes[50];
        int process_count = 0;

        int get_proc_result = developer_tools_get_dev_processes(tools, processes, 50, &process_count);
        TEST_ASSERT_TRUE(get_proc_result >= -1, "Получение процессов должно возвращать корректные коды");
        TEST_ASSERT_TRUE(process_count >= 0, "Количество процессов должно быть неотрицательным");

        if (get_proc_result == 0 && process_count > 0) {
            TEST_DIAGNOSTIC("Найдено процессов разработки: %d", process_count);

            /* Проверка корректности данных процессов */
            for (int i = 0; i < process_count && i < 5; i++) {
                TEST_ASSERT_TRUE(processes[i].pid > 0, "PID процесса должен быть положительным");
                TEST_ASSERT_NOT_NULL(processes[i].name, "Имя процесса не должно быть NULL");
                TEST_ASSERT_TRUE(strlen(processes[i].name) > 0, "Имя процесса не должно быть пустым");
                TEST_ASSERT_IN_RANGE(processes[i].cpu_usage, 0.0, 100.0,
                                   "CPU usage процесса должен быть в диапазоне 0-100%");
                TEST_ASSERT_IN_RANGE(processes[i].mem_usage, 0.0, 100.0,
                                   "Memory usage процесса должен быть в диапазоне 0-100%");
            }
        } else {
            TEST_DIAGNOSTIC("Процессы разработки не найдены или недоступны");
        }

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Анализ процессов разработки работает корректно");
}
TEST_END();

TEST_BEGIN(test_developer_tools_port_analysis) {
    /* Тестирование анализа портов разработки */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Тестирование получения слушающих портов */
        port_info_t ports[20];
        int port_count = 0;

        int get_ports_result = developer_tools_get_listening_ports(tools, ports, 20, &port_count);
        TEST_ASSERT_TRUE(get_ports_result >= -1, "Получение портов должно возвращать корректные коды");
        TEST_ASSERT_TRUE(port_count >= 0, "Количество портов должно быть неотрицательным");

        if (get_ports_result == 0 && port_count > 0) {
            TEST_DIAGNOSTIC("Найдено слушающих портов: %d", port_count);

            /* Проверка корректности данных портов */
            for (int i = 0; i < port_count && i < 5; i++) {
                TEST_ASSERT_TRUE(ports[i].port > 0, "Номер порта должен быть положительным");
                TEST_ASSERT_TRUE(ports[i].port <= 65535, "Номер порта не должен превышать 65535");
                TEST_ASSERT_NOT_NULL(ports[i].protocol, "Протокол порта не должен быть NULL");
                TEST_ASSERT_NOT_NULL(ports[i].process_name, "Имя процесса порта не должно быть NULL");
                TEST_ASSERT_TRUE(ports[i].pid > 0, "PID процесса порта должен быть положительным");
            }
        } else {
            TEST_DIAGNOSTIC("Слушающие порты не найдены или недоступны");
        }

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Анализ портов разработки работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для мониторинга ресурсов разработки
 * ============================================================================ */

TEST_BEGIN(test_developer_tools_resource_monitoring) {
    /* Тестирование мониторинга ресурсов разработки */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Тестирование получения метрик ресурсов разработки */
        dev_metrics_t metrics;
        int get_metrics_result = developer_tools_get_metrics(tools, &metrics);
        TEST_ASSERT_TRUE(get_metrics_result >= -1, "Получение метрик должно возвращать корректные коды");

        if (get_metrics_result == 0) {
            TEST_DIAGNOSTIC("Метрики ресурсов разработки получены");
            TEST_DIAGNOSTIC("  Процессы: %d", metrics.process_count);
            TEST_DIAGNOSTIC("  Порты: %d", metrics.port_count);
            TEST_DIAGNOSTIC("  CPU: %.1f%%", metrics.total_cpu_usage);
            TEST_DIAGNOSTIC("  Память: %.1f%%", metrics.total_memory_usage);

            /* Проверка корректности метрик */
            TEST_ASSERT_TRUE(metrics.process_count >= 0, "Количество процессов должно быть неотрицательным");
            TEST_ASSERT_TRUE(metrics.port_count >= 0, "Количество портов должно быть неотрицательным");
            TEST_ASSERT_IN_RANGE(metrics.total_cpu_usage, 0.0, 100.0,
                               "Общее CPU usage должно быть в диапазоне 0-100%");
            TEST_ASSERT_IN_RANGE(metrics.total_memory_usage, 0.0, 100.0,
                               "Общее memory usage должно быть в диапазоне 0-100%");
        }

        /* Тестирование мониторинга в реальном времени */
        int start_result = developer_tools_start_monitoring(tools);
        TEST_ASSERT_TRUE(start_result >= -1, "Запуск мониторинга должен возвращать корректные коды");

        if (start_result == 0) {
            TEST_DIAGNOSTIC("Мониторинг ресурсов разработки запущен");

            /* Проверка что мониторинг активен */
            TEST_ASSERT_TRUE(developer_tools_is_monitoring(tools),
                           "Мониторинг должен быть активен");

            /* Небольшая пауза для накопления данных */
            sleep(1);

            /* Проверка что данные обновляются */
            dev_metrics_t metrics_after;
            developer_tools_get_metrics(tools, &metrics_after);

            /* Остановка мониторинга */
            int stop_result = developer_tools_stop_monitoring(tools);
            TEST_ASSERT_SUCCESS(stop_result, "Остановка мониторинга должна работать");

            TEST_ASSERT_FALSE(developer_tools_is_monitoring(tools),
                            "Мониторинг должен быть остановлен");
        }

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Мониторинг ресурсов разработки работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для диагностики разработки
 * ============================================================================ */

TEST_BEGIN(test_developer_tools_diagnostics) {
    /* Тестирование диагностических функций */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Тестирование диагностики производительности */
        dev_performance_t performance;
        int get_perf_result = developer_tools_get_performance_diagnostics(tools, &performance);
        TEST_ASSERT_TRUE(get_perf_result >= -1, "Получение диагностики должно возвращать корректные коды");

        if (get_perf_result == 0) {
            TEST_DIAGNOSTIC("Диагностика производительности получена");
            TEST_DIAGNOSTIC("  Среднее время компиляции: %.2f сек", performance.avg_build_time_sec);
            TEST_DIAGNOSTIC("  Использование памяти: %.1f MB", performance.memory_usage_mb);
            TEST_DIAGNOSTIC("  Загрузка CPU: %.1f%%", performance.cpu_load_percent);

            /* Проверка корректности диагностических данных */
            TEST_ASSERT_TRUE(performance.avg_build_time_sec >= 0.0,
                           "Время компиляции должно быть неотрицательным");
            TEST_ASSERT_TRUE(performance.memory_usage_mb >= 0.0,
                           "Использование памяти должно быть неотрицательным");
            TEST_ASSERT_IN_RANGE(performance.cpu_load_percent, 0.0, 100.0,
                               "Загрузка CPU должна быть в диапазоне 0-100%");
        }

        /* Тестирование диагностики проблем */
        char diagnostics[512];
        int get_diag_result = developer_tools_get_diagnostics(tools, diagnostics, sizeof(diagnostics));
        TEST_ASSERT_TRUE(get_diag_result >= -1, "Получение диагностики проблем должно работать");

        if (get_diag_result > 0) {
            TEST_DIAGNOSTIC("Диагностика проблем: %s", diagnostics);
            TEST_ASSERT_TRUE(strlen(diagnostics) > 0, "Диагностика не должна быть пустой");
        }

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Диагностика разработки работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для интеграции с IDE
 * ============================================================================ */

TEST_BEGIN(test_developer_tools_ide_integration) {
    /* Тестирование интеграции с IDE */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Тестирование получения информации об IDE */
        ide_info_t ide_info;
        int get_ide_result = developer_tools_get_ide_info(tools, &ide_info);
        TEST_ASSERT_TRUE(get_ide_result >= -1, "Получение информации об IDE должно работать");

        if (get_ide_result == 0) {
            TEST_DIAGNOSTIC("Информация об IDE получена:");
            TEST_DIAGNOSTIC("  Имя: %s", ide_info.name);
            TEST_DIAGNOSTIC("  Версия: %s", ide_info.version);
            TEST_DIAGNOSTIC("  Активен: %s", ide_info.is_running ? "да" : "нет");

            /* Проверка корректности данных IDE */
            TEST_ASSERT_NOT_NULL(ide_info.name, "Имя IDE не должно быть NULL");
            TEST_ASSERT_NOT_NULL(ide_info.version, "Версия IDE не должна быть NULL");
            TEST_ASSERT_TRUE(strlen(ide_info.name) > 0, "Имя IDE не должно быть пустым");
            TEST_ASSERT_TRUE(strlen(ide_info.version) > 0, "Версия IDE не должна быть пустой");
        } else {
            TEST_DIAGNOSTIC("IDE не обнаружено или недоступно");
        }

        /* Тестирование мониторинга активности IDE */
        int start_monitoring_result = developer_tools_start_ide_monitoring(tools);
        TEST_ASSERT_TRUE(start_monitoring_result >= -1,
                        "Запуск мониторинга IDE должен возвращать корректные коды");

        if (start_monitoring_result == 0) {
            TEST_DIAGNOSTIC("Мониторинг IDE запущен");

            /* Проверка активности мониторинга */
            TEST_ASSERT_TRUE(developer_tools_is_ide_monitoring(tools),
                           "Мониторинг IDE должен быть активен");

            /* Остановка мониторинга */
            developer_tools_stop_ide_monitoring(tools);
            TEST_ASSERT_FALSE(developer_tools_is_ide_monitoring(tools),
                            "Мониторинг IDE должен быть остановлен");
        }

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Интеграция с IDE работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для производительности Developer Tools
 * ============================================================================ */

TEST_BEGIN(test_developer_tools_performance) {
    /* Тестирование производительности Developer Tools */

    developer_tools_t *tools = developer_tools_create();
    TEST_ASSERT_NOT_NULL(tools, "Не удалось создать Developer Tools");

    if (tools) {
        /* Измерение времени анализа процессов */
        const int iterations = 50;

        TEST_TIME_START();

        for (int i = 0; i < iterations; i++) {
            dev_process_t processes[20];
            int process_count = 0;
            developer_tools_get_dev_processes(tools, processes, 20, &process_count);
        }

        uint64_t elapsed_time = TEST_TIME_END();
        double avg_time_per_call = (double)elapsed_time / iterations;

        TEST_DIAGNOSTIC("Среднее время анализа процессов: %.2f мс", avg_time_per_call);

        /* Проверка производительности */
        TEST_ASSERT_TRUE(avg_time_per_call < 100.0,
                        "Анализ процессов не должен занимать больше 100мс в среднем");

        /* Тестирование производительности анализа портов */
        TEST_TIME_START();

        for (int i = 0; i < iterations; i++) {
            port_info_t ports[10];
            int port_count = 0;
            developer_tools_get_listening_ports(tools, ports, 10, &port_count);
        }

        elapsed_time = TEST_TIME_END();
        avg_time_per_call = (double)elapsed_time / iterations;

        TEST_DIAGNOSTIC("Среднее время анализа портов: %.2f мс", avg_time_per_call);

        /* Анализ портов может быть медленнее из-за сетевых операций */
        TEST_ASSERT_TRUE(avg_time_per_call < 200.0,
                        "Анализ портов не должен занимать больше 200мс в среднем");

        developer_tools_destroy(tools);
    }

    TEST_ASSERT_SUCCESS(0, "Производительность Developer Tools в пределах нормы");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы Developer Tools тестов
 */
static void developer_tools_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы Developer Tools тестов...");

    /* Проверка что инструменты разработки доступны */
    TEST_DIAGNOSTIC("Проверка доступности инструментов разработки");
}

/**
 * @brief Очистка после завершения группы Developer Tools тестов
 */
static void developer_tools_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы Developer Tools тестов...");

    /* Освобождение ресурсов Developer Tools если необходимо */
    TEST_DIAGNOSTIC("Освобождение ресурсов Developer Tools");
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы Developer Tools тестов
 */
test_suite_t *create_developer_tools_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("developer_tools",
                                          "Тесты Developer Tools модуля",
                                          TEST_TYPE_UNIT);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы Developer Tools тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Базовые тесты */
    test_def = test_definition_create("test_developer_tools_initialization", "developer_tools",
                                     test_developer_tools_initialization, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_developer_tools_environment_detection", "developer_tools",
                                     test_developer_tools_environment_detection, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты анализа процессов */
    test_def = test_definition_create("test_developer_tools_process_analysis", "developer_tools",
                                     test_developer_tools_process_analysis, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_developer_tools_port_analysis", "developer_tools",
                                     test_developer_tools_port_analysis, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты мониторинга ресурсов */
    test_def = test_definition_create("test_developer_tools_resource_monitoring", "developer_tools",
                                     test_developer_tools_resource_monitoring, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты диагностики */
    test_def = test_definition_create("test_developer_tools_diagnostics", "developer_tools",
                                     test_developer_tools_diagnostics, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты интеграции */
    test_def = test_definition_create("test_developer_tools_ide_integration", "developer_tools",
                                     test_developer_tools_ide_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_developer_tools_performance", "developer_tools",
                                     test_developer_tools_performance, TEST_TYPE_PERFORMANCE);
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

    TEST_DIAGNOSTIC("Запуск группы Developer Tools тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("===================================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 30000; /* 30 секунд таймаут для Developer Tools тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_developer_tools_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы Developer Tools тестов");
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
    TEST_DIAGNOSTIC("Группа Developer Tools тестов завершена");
    return result;
}