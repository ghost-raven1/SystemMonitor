/**
 * @file test_core_modules.c
 * @brief Unit тесты для core модулей в новом тестовом фреймворке
 *
 * Адаптированные версии существующих core тестов для использования
 * с новым тестовым фреймворком SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/platform/system_info.h"
#include "../../../src/platform/processes.h"
#include "../../../src/platform/network.h"
#include "../../../src/platform/disk.h"
#include "../../../src/utils/logging.h"
#include "../../../src/platform/battery.h"
#include "../../../src/platform/gpu.h"
#include "../../../src/core/developer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования core модулей
 */
typedef struct {
    char module_name[64];
    int test_iterations;
    bool performance_monitoring;
} core_test_data_t;

/**
 * @brief Функция создания тестовых данных core модулей
 */
core_test_data_t *create_core_test_data(const char *module_name) {
    core_test_data_t *data = malloc(sizeof(core_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(core_test_data_t));
    safe_strcpy(data->module_name, module_name, sizeof(data->module_name));
    data->test_iterations = 5;
    data->performance_monitoring = true;

    return data;
}

/**
 * @brief Функция освобождения тестовых данных core модулей
 */
void destroy_core_test_data(core_test_data_t *data) {
    if (data) {
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для system_info модуля
 * ============================================================================ */

TEST_BEGIN(test_system_info_basic) {
    /* Тестирование базового функционала system_info модуля */

    /* Тестирование CPU usage */
    float cpu_usage = get_cpu_usage();
    TEST_ASSERT_IN_RANGE(cpu_usage, 0.0, 100.0, "CPU usage должен быть в диапазоне 0-100%");
    TEST_DIAGNOSTIC("Текущее использование CPU: %.1f%%", cpu_usage);

    /* Тестирование memory usage */
    float memory_usage = get_memory_usage();
    TEST_ASSERT_IN_RANGE(memory_usage, 0.0, 100.0, "Memory usage должен быть в диапазоне 0-100%");
    TEST_DIAGNOSTIC("Текущее использование памяти: %.1f%%", memory_usage);

    /* Тестирование uptime */
    long uptime = get_uptime_seconds();
    TEST_ASSERT_TRUE(uptime >= 0, "Uptime должен быть неотрицательным");
    TEST_DIAGNOSTIC("Время работы системы: %ld секунд", uptime);

    TEST_ASSERT_SUCCESS(0, "Базовый функционал system_info работает корректно");
}
TEST_END();

TEST_BEGIN(test_system_info_cpu_frequencies) {
    /* Тестирование получения CPU частот */

    long long current_hz, max_hz;

    int result = get_cpu_frequencies(&current_hz, &max_hz);
    TEST_ASSERT_SUCCESS(result, "get_cpu_frequencies должен выполняться успешно");

    if (result == 0) {
        TEST_ASSERT_TRUE(current_hz > 0, "Текущая частота CPU должна быть положительной");
        TEST_ASSERT_TRUE(max_hz > 0, "Максимальная частота CPU должна быть положительной");
        TEST_ASSERT_TRUE(current_hz <= max_hz, "Текущая частота не может превышать максимальную");

        TEST_DIAGNOSTIC("CPU частоты - текущая: %lld Hz, максимальная: %lld Hz",
                       current_hz, max_hz);
    }

    TEST_ASSERT_SUCCESS(0, "CPU частоты получены корректно");
}
TEST_END();

TEST_BEGIN(test_system_info_per_core_usage) {
    /* Тестирование получения использования CPU по ядрам */

    float cores[32];
    int written = 0;

    int result = get_per_core_usage(cores, 32, &written);
    TEST_ASSERT_SUCCESS(result, "get_per_core_usage должен выполняться успешно");

    if (result == 0) {
        TEST_ASSERT_TRUE(written > 0, "Должно быть записано хотя бы одно значение");
        TEST_ASSERT_TRUE(written <= 32, "Не должно быть записано больше 32 значений");

        /* Проверка корректности данных для каждого ядра */
        for (int i = 0; i < written && i < 10; i++) {
            TEST_ASSERT_IN_RANGE(cores[i], 0.0, 100.0,
                               "Использование ядра должно быть в диапазоне 0-100%");
        }

        TEST_DIAGNOSTIC("Использование CPU по ядрам получено: %d ядер", written);
    }

    TEST_ASSERT_SUCCESS(0, "Per-core CPU usage работает корректно");
}
TEST_END();

TEST_BEGIN(test_system_info_memory_breakdown) {
    /* Тестирование получения разбивки памяти */

    memory_breakdown_t memory_breakdown;

    int result = get_memory_breakdown(&memory_breakdown);
    TEST_ASSERT_SUCCESS(result, "get_memory_breakdown должен выполняться успешно");

    if (result == 0) {
        TEST_ASSERT_TRUE(memory_breakdown.total >= 0, "Общая память должна быть неотрицательной");
        TEST_ASSERT_TRUE(memory_breakdown.used >= 0, "Используемая память должна быть неотрицательной");
        TEST_ASSERT_TRUE(memory_breakdown.free >= 0, "Свободная память должна быть неотрицательной");
        TEST_ASSERT_TRUE(memory_breakdown.available >= 0, "Доступная память должна быть неотрицательной");

        /* Проверка что свободная + используемая ≈ общая */
        unsigned long long sum = memory_breakdown.free + memory_breakdown.used;
        TEST_ASSERT_TRUE(abs((long long)sum - (long long)memory_breakdown.total) < memory_breakdown.total * 0.1,
                        "Свободная + используемая память должна примерно равняться общей");

        TEST_DIAGNOSTIC("Память - общая: %llu MB, используемая: %llu MB, свободная: %llu MB",
                       memory_breakdown.total / (1024*1024),
                       memory_breakdown.used / (1024*1024),
                       memory_breakdown.free / (1024*1024));
    }

    TEST_ASSERT_SUCCESS(0, "Разбивка памяти получена корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для processes модуля
 * ============================================================================ */

TEST_BEGIN(test_processes_basic) {
    /* Тестирование базового функционала processes модуля */

    process_info_t processes[100];
    size_t count = get_process_list(processes, 100);

    TEST_ASSERT_TRUE(count > 0, "Должен быть найден хотя бы один процесс");
    TEST_ASSERT_TRUE(count <= 100, "Количество процессов не должно превышать размер буфера");

    TEST_DIAGNOSTIC("Найдено процессов: %zu", count);

    /* Проверка корректности данных процессов */
    for (size_t i = 0; i < count && i < 10; i++) {
        TEST_ASSERT_TRUE(processes[i].pid > 0, "PID процесса должен быть положительным");
        TEST_ASSERT_NOT_NULL(processes[i].name, "Имя процесса не должно быть NULL");
        TEST_ASSERT_TRUE(strlen(processes[i].name) > 0, "Имя процесса не должно быть пустым");
        TEST_ASSERT_IN_RANGE(processes[i].cpu_usage, 0.0, 100.0, "CPU usage процесса должен быть в диапазоне 0-100%");
        TEST_ASSERT_IN_RANGE(processes[i].mem_usage, 0.0, 100.0, "Memory usage процесса должен быть в диапазоне 0-100%");
    }

    TEST_ASSERT_SUCCESS(0, "Базовый функционал processes работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для network модуля
 * ============================================================================ */

TEST_BEGIN(test_network_basic) {
    /* Тестирование базового функционала network модуля */

    /* Тестирование получения сетевой информации */
    const char *network_info = get_network_info();
    TEST_ASSERT_NOT_NULL(network_info, "Сетевая информация не должна быть NULL");

    if (network_info) {
        TEST_ASSERT_TRUE(strlen(network_info) > 0, "Сетевая информация не должна быть пустой");
        TEST_DIAGNOSTIC("Сетевая информация: %s", network_info);
    }

    /* Тестирование получения RX/TX скоростей */
    float rx_rate = get_network_rx();
    float tx_rate = get_network_tx();

    TEST_ASSERT_TRUE(rx_rate >= 0.0, "RX скорость должна быть неотрицательной");
    TEST_ASSERT_TRUE(tx_rate >= 0.0, "TX скорость должна быть неотрицательной");

    TEST_DIAGNOSTIC("Сетевые скорости - RX: %.2f MB/s, TX: %.2f MB/s", rx_rate, tx_rate);

    TEST_ASSERT_SUCCESS(0, "Базовый функционал network работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для disk модуля
 * ============================================================================ */

TEST_BEGIN(test_disk_basic) {
    /* Тестирование базового функционала disk модуля */

    /* Тестирование получения информации о диске */
    const char *disk_info = get_disk_info();
    TEST_ASSERT_NOT_NULL(disk_info, "Информация о диске не должна быть NULL");

    if (disk_info) {
        TEST_ASSERT_TRUE(strlen(disk_info) > 0, "Информация о диске не должна быть пустой");
        TEST_DIAGNOSTIC("Информация о диске: %s", disk_info);
    }

    /* Тестирование получения использования диска для корневой файловой системы */
    float root_usage = get_disk_usage("/");
    TEST_ASSERT_IN_RANGE(root_usage, 0.0, 100.0, "Использование корневого диска должно быть в диапазоне 0-100%");
    TEST_DIAGNOSTIC("Использование корневого диска: %.1f%%", root_usage);

    /* Тестирование с некорректным путем */
    float invalid_usage = get_disk_usage("/nonexistent/path");
    TEST_ASSERT_TRUE(invalid_usage == 0.0, "Некорректный путь должен возвращать 0%");

    TEST_ASSERT_SUCCESS(0, "Базовый функционал disk работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для logging модуля
 * ============================================================================ */

TEST_BEGIN(test_logging_basic) {
    /* Тестирование базового функционала logging модуля */

    /* Тестирование базового логирования */
    TEST_DIAGNOSTIC("Начинаем тестирование логирования");

    /* Тест что логирование не вызывает падений */
    log_info("Test info message from unit test");
    log_error("Test error message from unit test");

    TEST_DIAGNOSTIC("Логирование выполнено успешно");

    TEST_ASSERT_SUCCESS(0, "Базовый функционал logging работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для battery модуля (интеграция с core)
 * ============================================================================ */

TEST_BEGIN(test_battery_integration) {
    /* Тестирование интеграции battery модуля с core */

    battery_info_t battery_info;
    int result = get_battery_info(&battery_info);

    /* Не обязательно должна быть батарея, но функция не должна падать */
    TEST_ASSERT_TRUE(result >= -1, "get_battery_info должен возвращать корректные коды ошибок");

    if (result == 0) {
        TEST_DIAGNOSTIC("Интеграция с батареей: %.1f%%, зарядка: %s",
                       battery_info.percentage,
                       battery_info.charging ? "да" : "нет");

        /* Проверка что данные можно использовать в core системе */
        TEST_ASSERT_IN_RANGE(battery_info.percentage, 0.0, 100.0,
                           "Данные батареи должны быть в корректном диапазоне");
    }

    TEST_ASSERT_SUCCESS(0, "Интеграция battery модуля работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для GPU модуля (интеграция с core)
 * ============================================================================ */

TEST_BEGIN(test_gpu_integration) {
    /* Тестирование интеграции GPU модуля с core */

    gpu_info_t gpu_info;
    int result = get_gpu_info(&gpu_info);

    /* Функция должна работать без ошибок */
    TEST_ASSERT_TRUE(result >= -1, "get_gpu_info должен возвращать корректные коды ошибок");

    if (result == 0) {
        TEST_DIAGNOSTIC("Интеграция с GPU: температура=%.1f°C, память=%dMB",
                       gpu_info.temperature, gpu_info.memory_mb);

        /* Проверка корректности данных */
        TEST_ASSERT_TRUE(gpu_info.temperature >= 0.0, "Температура GPU должна быть неотрицательной");
        TEST_ASSERT_TRUE(gpu_info.memory_mb >= 0, "Память GPU должна быть неотрицательной");
    }

    TEST_ASSERT_SUCCESS(0, "Интеграция GPU модуля работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для developer модуля (интеграция с core)
 * ============================================================================ */

TEST_BEGIN(test_developer_integration) {
    /* Тестирование интеграции developer модуля с core */

    /* Тест портов */
    port_info_t ports[10];
    int port_count = 0;

    int result_ports = get_listening_ports(ports, 10, &port_count);
    TEST_ASSERT_SUCCESS(result_ports, "get_listening_ports должен выполняться успешно");
    TEST_ASSERT_TRUE(port_count >= 0, "Количество портов должно быть неотрицательным");

    if (port_count > 0) {
        TEST_DIAGNOSTIC("Найдено слушающих портов: %d", port_count);
    }

    /* Тест окружения разработки */
    dev_environment_t dev_env;
    int result_env = get_dev_environment(&dev_env);
    TEST_ASSERT_SUCCESS(result_env, "get_dev_environment должен выполняться успешно");

    /* Тест процессов разработки */
    dev_process_t dev_processes[10];
    int dev_proc_count = 0;

    int result_dev_proc = get_dev_processes(dev_processes, 10, &dev_proc_count);
    TEST_ASSERT_SUCCESS(result_dev_proc, "get_dev_processes должен выполняться успешно");
    TEST_ASSERT_TRUE(dev_proc_count >= 0, "Количество процессов разработки должно быть неотрицательным");

    TEST_ASSERT_SUCCESS(0, "Интеграция developer модуля работает корректно");
}
TEST_END();

/* ============================================================================
 * Интеграционные тесты между core модулями
 * ============================================================================ */

TEST_BEGIN(test_core_modules_integration) {
    /* Тестирование интеграции между core модулями */

    /* Тестирование что все модули могут работать параллельно */
    float cpu_before = get_cpu_usage();
    float mem_before = get_memory_usage();

    /* Запуск нескольких операций одновременно */
    process_info_t processes[50];
    size_t proc_count = get_process_list(processes, 50);

    battery_info_t battery;
    get_battery_info(&battery);

    float rx = get_network_rx();
    float tx = get_network_tx();

    float cpu_after = get_cpu_usage();
    float mem_after = get_memory_usage();

    /* Проверка что все операции завершились успешно */
    TEST_ASSERT_TRUE(proc_count > 0, "Должен быть найден хотя бы один процесс");
    TEST_ASSERT_TRUE(rx >= 0.0 && tx >= 0.0, "Сетевые скорости должны быть корректными");
    TEST_ASSERT_IN_RANGE(cpu_after, 0.0, 100.0, "CPU usage после операций");
    TEST_ASSERT_IN_RANGE(mem_after, 0.0, 100.0, "Memory usage после операций");

    /* Проверка что нагрузка от тестирования разумная */
    float cpu_diff = cpu_after - cpu_before;
    float mem_diff = mem_after - mem_before;

    TEST_ASSERT_TRUE(cpu_diff >= -10.0 && cpu_diff <= 50.0,
                    "Разница CPU usage должна быть в разумных пределах");
    TEST_ASSERT_TRUE(mem_diff >= -10.0 && mem_diff <= 20.0,
                    "Разница memory usage должна быть в разумных пределах");

    TEST_DIAGNOSTIC("Интеграционное тестирование завершено - CPU: %.1f%%, Memory: %.1f%%",
                   cpu_after, mem_after);

    TEST_ASSERT_SUCCESS(0, "Интеграция core модулей работает корректно");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы core тестов
 */
static void core_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы core тестов...");

    /* Инициализация логирования для тестов */
    TEST_DIAGNOSTIC("Инициализация core модулей для тестирования");
}

/**
 * @brief Очистка после завершения группы core тестов
 */
static void core_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы core тестов...");

    /* Освобождение ресурсов core модулей если необходимо */
    TEST_DIAGNOSTIC("Освобождение ресурсов core модулей");
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы core тестов
 */
test_suite_t *create_core_modules_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("core_modules",
                                          "Тесты core модулей системы мониторинга",
                                          TEST_TYPE_UNIT);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы core тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Тесты system_info модуля */
    test_def = test_definition_create("test_system_info_basic", "core_modules",
                                     test_system_info_basic, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_info_cpu_frequencies", "core_modules",
                                     test_system_info_cpu_frequencies, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_info_per_core_usage", "core_modules",
                                     test_system_info_per_core_usage, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_info_memory_breakdown", "core_modules",
                                     test_system_info_memory_breakdown, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты processes модуля */
    test_def = test_definition_create("test_processes_basic", "core_modules",
                                     test_processes_basic, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты network модуля */
    test_def = test_definition_create("test_network_basic", "core_modules",
                                     test_network_basic, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты disk модуля */
    test_def = test_definition_create("test_disk_basic", "core_modules",
                                     test_disk_basic, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты logging модуля */
    test_def = test_definition_create("test_logging_basic", "core_modules",
                                     test_logging_basic, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Интеграционные тесты */
    test_def = test_definition_create("test_battery_integration", "core_modules",
                                     test_battery_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_gpu_integration", "core_modules",
                                     test_gpu_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_developer_integration", "core_modules",
                                     test_developer_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_core_modules_integration", "core_modules",
                                     test_core_modules_integration, TEST_TYPE_INTEGRATION);
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

    TEST_DIAGNOSTIC("Запуск группы core модулей тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("================================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 60000; /* 60 секунд таймаут для core тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_core_modules_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы core тестов");
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
    TEST_DIAGNOSTIC("Группа core модулей тестов завершена");
    return result;
}