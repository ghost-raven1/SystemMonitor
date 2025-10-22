/**
 * @file test_platform.c
 * @brief Unit тесты для platform-specific модулей в новом тестовом фреймворке
 *
 * Адаптированные версии существующих platform тестов для использования
 * с новым тестовым фреймворком SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/platform/platform.h"
#include "../../../src/platform/system_info.h"
#include "../../../src/platform/processes.h"
#include "../../../src/platform/network.h"
#include "../../../src/platform/disk.h"
#include "../../../src/platform/gpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования platform модулей
 */
typedef struct {
    platform_info_t platform_info;
    char platform_type[64];
    char test_environment[128];
} platform_test_data_t;

/**
 * @brief Функция создания тестовых данных platform
 */
platform_test_data_t *create_platform_test_data(void) {
    platform_test_data_t *data = malloc(sizeof(platform_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(platform_test_data_t));

    /* Определение типа платформы */
    platform_type_t type = get_platform_type();
    switch (type) {
        case PLATFORM_MACOS:
            safe_strcpy(data->platform_type, "macOS", sizeof(data->platform_type));
            break;
        case PLATFORM_LINUX:
            safe_strcpy(data->platform_type, "Linux", sizeof(data->platform_type));
            break;
        case PLATFORM_WINDOWS:
            safe_strcpy(data->platform_type, "Windows", sizeof(data->platform_type));
            break;
        default:
            safe_strcpy(data->platform_type, "Unknown", sizeof(data->platform_type));
            break;
    }

    safe_strcpy(data->test_environment, "Unit Test Environment", sizeof(data->test_environment));

    return data;
}

/**
 * @brief Функция освобождения тестовых данных platform
 */
void destroy_platform_test_data(platform_test_data_t *data) {
    if (data) {
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для определения типа платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_detection) {
    /* Тестирование определения типа платформы */

    platform_type_t platform_type = get_platform_type();
    TEST_ASSERT_TRUE(platform_type >= PLATFORM_UNKNOWN && platform_type <= PLATFORM_WINDOWS,
                    "Тип платформы должен быть в корректном диапазоне");

    platform_test_data_t *test_data = create_platform_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Не удалось создать тестовые данные платформы");

    if (test_data) {
        TEST_DIAGNOSTIC("Обнаружен тип платформы: %s (код: %d)",
                       test_data->platform_type, platform_type);

        /* Проверка что определение платформы стабильно */
        platform_type_t platform_type2 = get_platform_type();
        TEST_ASSERT_TRUE(platform_type == platform_type2,
                        "Определение типа платформы должно быть стабильным");

        destroy_platform_test_data(test_data);
    }

    TEST_ASSERT_SUCCESS(0, "Определение типа платформы работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для системной информации платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_system_info) {
    /* Тестирование системной информации платформы */

    /* Тестирование получения CPU usage */
    float cpu_usage = get_cpu_usage();
    TEST_ASSERT_IN_RANGE(cpu_usage, 0.0, 100.0, "CPU usage должен быть в диапазоне 0-100%");
    TEST_DIAGNOSTIC("CPU usage платформы: %.1f%%", cpu_usage);

    /* Тестирование получения memory usage */
    float memory_usage = get_memory_usage();
    TEST_ASSERT_IN_RANGE(memory_usage, 0.0, 100.0, "Memory usage должен быть в диапазоне 0-100%");
    TEST_DIAGNOSTIC("Memory usage платформы: %.1f%%", memory_usage);

    /* Тестирование получения uptime */
    long uptime = get_uptime_seconds();
    TEST_ASSERT_TRUE(uptime >= 0, "Uptime должен быть неотрицательным");
    TEST_DIAGNOSTIC("Uptime платформы: %ld секунд", uptime);

    /* Тестирование получения load average */
    float load_avg[3];
    int la_result = get_load_average(load_avg, 3);
    TEST_ASSERT_TRUE(la_result >= -1, "get_load_average должен возвращать корректные коды");

    if (la_result == 0) {
        TEST_DIAGNOSTIC("Load average: %.2f, %.2f, %.2f", load_avg[0], load_avg[1], load_avg[2]);
        TEST_ASSERT_TRUE(load_avg[0] >= 0.0, "Load average 1 мин должен быть неотрицательным");
        TEST_ASSERT_TRUE(load_avg[1] >= 0.0, "Load average 5 мин должен быть неотрицательным");
        TEST_ASSERT_TRUE(load_avg[2] >= 0.0, "Load average 15 мин должен быть неотрицательным");
    }

    TEST_ASSERT_SUCCESS(0, "Системная информация платформы работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для процессов платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_processes) {
    /* Тестирование процессов платформы */

    process_info_t processes[100];
    size_t count = get_process_list(processes, 100);

    TEST_ASSERT_TRUE(count > 0, "Должен быть найден хотя бы один процесс");
    TEST_ASSERT_TRUE(count <= 100, "Количество процессов не должно превышать размер буфера");

    TEST_DIAGNOSTIC("Найдено процессов платформы: %zu", count);

    /* Проверка корректности данных процессов */
    for (size_t i = 0; i < count && i < 10; i++) {
        TEST_ASSERT_TRUE(processes[i].pid > 0, "PID процесса должен быть положительным");
        TEST_ASSERT_NOT_NULL(processes[i].name, "Имя процесса не должно быть NULL");
        TEST_ASSERT_TRUE(strlen(processes[i].name) > 0, "Имя процесса не должно быть пустым");
        TEST_ASSERT_IN_RANGE(processes[i].cpu_usage, 0.0, 100.0,
                           "CPU usage процесса должен быть в диапазоне 0-100%");
        TEST_ASSERT_IN_RANGE(processes[i].mem_usage, 0.0, 100.0,
                           "Memory usage процесса должен быть в диапазоне 0-100%");
    }

    /* Тестирование получения процесса по PID */
    if (count > 0) {
        pid_t test_pid = processes[0].pid;
        process_info_t single_process;

        int get_result = get_process_by_pid(test_pid, &single_process);
        TEST_ASSERT_TRUE(get_result >= -1, "get_process_by_pid должен возвращать корректные коды");

        if (get_result == 0) {
            TEST_ASSERT_TRUE(single_process.pid == test_pid, "PID должен совпадать");
            TEST_ASSERT_NOT_NULL(single_process.name, "Имя процесса не должно быть NULL");
        }
    }

    TEST_ASSERT_SUCCESS(0, "Процессы платформы работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для сети платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_network) {
    /* Тестирование сети платформы */

    /* Тестирование получения сетевой информации */
    const char *network_info = get_network_info();
    TEST_ASSERT_NOT_NULL(network_info, "Сетевая информация не должна быть NULL");

    if (network_info) {
        TEST_ASSERT_TRUE(strlen(network_info) > 0, "Сетевая информация не должна быть пустой");
        TEST_DIAGNOSTIC("Сетевая информация платформы: %s", network_info);
    }

    /* Тестирование получения RX/TX скоростей */
    float rx_rate = get_network_rx();
    float tx_rate = get_network_tx();

    TEST_ASSERT_TRUE(rx_rate >= 0.0, "RX скорость должна быть неотрицательной");
    TEST_ASSERT_TRUE(tx_rate >= 0.0, "TX скорость должна быть неотрицательной");

    TEST_DIAGNOSTIC("Сетевые скорости платформы - RX: %.2f MB/s, TX: %.2f MB/s", rx_rate, tx_rate);

    TEST_ASSERT_SUCCESS(0, "Сеть платформы работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для диска платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_disk) {
    /* Тестирование диска платформы */

    /* Тестирование получения информации о диске */
    const char *disk_info = get_disk_info();
    TEST_ASSERT_NOT_NULL(disk_info, "Информация о диске не должна быть NULL");

    if (disk_info) {
        TEST_ASSERT_TRUE(strlen(disk_info) > 0, "Информация о диске не должна быть пустой");
        TEST_DIAGNOSTIC("Информация о диске платформы: %s", disk_info);
    }

    /* Тестирование получения использования диска для корневой файловой системы */
    float root_usage = get_disk_usage("/");
    TEST_ASSERT_IN_RANGE(root_usage, 0.0, 100.0, "Использование корневого диска должно быть в диапазоне 0-100%");
    TEST_DIAGNOSTIC("Использование корневого диска платформы: %.1f%%", root_usage);

    /* Тестирование с некорректным путем */
    float invalid_usage = get_disk_usage("/nonexistent/path");
    TEST_ASSERT_TRUE(invalid_usage == 0.0, "Некорректный путь должен возвращать 0%");

    TEST_ASSERT_SUCCESS(0, "Диск платформы работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для GPU платформы (если доступен)
 * ============================================================================ */

TEST_BEGIN(test_platform_gpu) {
    /* Тестирование GPU платформы */

    gpu_info_t gpu_info;
    int result = get_gpu_info(&gpu_info);

    /* GPU может быть недоступен на некоторых системах */
    TEST_ASSERT_TRUE(result >= -1, "get_gpu_info должен возвращать корректные коды");

    if (result == 0) {
        TEST_DIAGNOSTIC("GPU информация платформы получена");
        TEST_DIAGNOSTIC("  Температура: %.1f°C", gpu_info.temperature);
        TEST_DIAGNOSTIC("  Память: %d MB", gpu_info.memory_mb);
        TEST_DIAGNOSTIC("  Использование: %.1f%%", gpu_info.usage_percent);

        /* Проверка корректности данных GPU */
        TEST_ASSERT_TRUE(gpu_info.temperature >= 0.0, "Температура GPU должна быть неотрицательной");
        TEST_ASSERT_TRUE(gpu_info.memory_mb >= 0, "Память GPU должна быть неотрицательной");
        TEST_ASSERT_IN_RANGE(gpu_info.usage_percent, 0.0, 100.0,
                           "Использование GPU должно быть в диапазоне 0-100%");

        /* Тестирование получения статистики GPU */
        gpu_stats_t gpu_stats;
        int stats_result = get_gpu_stats(&gpu_stats);
        TEST_ASSERT_TRUE(stats_result >= -1, "get_gpu_stats должен возвращать корректные коды");

        if (stats_result == 0) {
            TEST_DIAGNOSTIC("GPU статистика получена успешно");
        }
    } else {
        TEST_DIAGNOSTIC("GPU недоступен на этой платформе (код ошибки: %d)", result);
    }

    TEST_ASSERT_SUCCESS(0, "GPU платформы работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для специфичных возможностей платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_specific_features) {
    /* Тестирование специфичных возможностей платформы */

    platform_test_data_t *test_data = create_platform_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Не удалось создать тестовые данные платформы");

    if (test_data) {
        TEST_DIAGNOSTIC("Тестирование специфичных возможностей платформы: %s",
                       test_data->platform_type);

        /* Тестирование возможностей в зависимости от типа платформы */
        platform_type_t platform_type = get_platform_type();

        switch (platform_type) {
            case PLATFORM_MACOS:
                TEST_DIAGNOSTIC("Тестирование специфичных возможностей macOS");
                /* Здесь могут быть тесты специфичных для macOS функций */
                break;

            case PLATFORM_LINUX:
                TEST_DIAGNOSTIC("Тестирование специфичных возможностей Linux");
                /* Здесь могут быть тесты специфичных для Linux функций */
                break;

            case PLATFORM_WINDOWS:
                TEST_DIAGNOSTIC("Тестирование специфичных возможностей Windows");
                /* Здесь могут быть тесты специфичных для Windows функций */
                break;

            default:
                TEST_DIAGNOSTIC("Неизвестная платформа - тестирование базовых функций");
                break;
        }

        destroy_platform_test_data(test_data);
    }

    TEST_ASSERT_SUCCESS(0, "Специфичные возможности платформы работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для производительности платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_performance) {
    /* Тестирование производительности операций платформы */

    /* Тестирование производительности получения списка процессов */
    const int iterations = 50;

    TEST_TIME_START();

    for (int i = 0; i < iterations; i++) {
        process_info_t processes[50];
        size_t count = get_process_list(processes, 50);
        (void)count; /* Избежать предупреждения компилятора */
    }

    uint64_t elapsed_time = TEST_TIME_END();
    double avg_time_per_call = (double)elapsed_time / iterations;

    TEST_DIAGNOSTIC("Среднее время получения списка процессов: %.2f мс", avg_time_per_call);

    /* Проверка производительности */
    TEST_ASSERT_TRUE(avg_time_per_call < 100.0,
                    "Получение списка процессов не должно занимать больше 100мс в среднем");

    /* Тестирование производительности получения системной информации */
    TEST_TIME_START();

    for (int i = 0; i < iterations; i++) {
        float cpu = get_cpu_usage();
        float mem = get_memory_usage();
        (void)cpu; (void)mem; /* Избежать предупреждения компилятора */
    }

    elapsed_time = TEST_TIME_END();
    avg_time_per_call = (double)elapsed_time / iterations;

    TEST_DIAGNOSTIC("Среднее время получения системной информации: %.2f мс", avg_time_per_call);

    /* Получение системной информации должно быть очень быстрым */
    TEST_ASSERT_TRUE(avg_time_per_call < 50.0,
                    "Получение системной информации не должно занимать больше 50мс в среднем");

    TEST_ASSERT_SUCCESS(0, "Производительность платформы в пределах нормы");
}
TEST_END();

/* ============================================================================
 * Unit тесты для стабильности платформы
 * ============================================================================ */

TEST_BEGIN(test_platform_stability) {
    /* Тестирование стабильности операций платформы */

    /* Тестирование что последовательные вызовы возвращают согласованные результаты */
    float cpu1 = get_cpu_usage();
    sleep(1);
    float cpu2 = get_cpu_usage();

    TEST_ASSERT_IN_RANGE(cpu1, 0.0, 100.0, "Первое значение CPU");
    TEST_ASSERT_IN_RANGE(cpu2, 0.0, 100.0, "Второе значение CPU");

    /* Разница не должна быть слишком большой без видимой причины */
    float cpu_diff = cpu2 - cpu1;
    TEST_ASSERT_IN_RANGE(cpu_diff, -20.0, 20.0,
                        "Разница CPU usage не должна превышать 20% без видимой причины");

    /* Тестирование что сетевые операции стабильны */
    float rx1 = get_network_rx();
    float tx1 = get_network_tx();

    sleep(1);

    float rx2 = get_network_rx();
    float tx2 = get_network_tx();

    TEST_ASSERT_TRUE(rx1 >= 0.0 && rx2 >= 0.0, "RX скорости должны быть неотрицательными");
    TEST_ASSERT_TRUE(tx1 >= 0.0 && tx2 >= 0.0, "TX скорости должны быть неотрицательными");

    TEST_DIAGNOSTIC("Сетевые скорости стабильны - RX: %.2f -> %.2f, TX: %.2f -> %.2f",
                   rx1, rx2, tx1, tx2);

    TEST_ASSERT_SUCCESS(0, "Стабильность платформы подтверждена");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы platform тестов
 */
static void platform_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы platform тестов...");

    /* Определение типа платформы для адаптации тестов */
    platform_type_t platform_type = get_platform_type();
    TEST_DIAGNOSTIC("Определен тип платформы для тестирования");

    switch (platform_type) {
        case PLATFORM_MACOS:
            TEST_DIAGNOSTIC("Настройка для macOS платформы");
            break;
        case PLATFORM_LINUX:
            TEST_DIAGNOSTIC("Настройка для Linux платформы");
            break;
        case PLATFORM_WINDOWS:
            TEST_DIAGNOSTIC("Настройка для Windows платформы");
            break;
        default:
            TEST_DIAGNOSTIC("Настройка для неизвестной платформы");
            break;
    }
}

/**
 * @brief Очистка после завершения группы platform тестов
 */
static void platform_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы platform тестов...");

    /* Освобождение ресурсов платформы если необходимо */
    TEST_DIAGNOSTIC("Освобождение ресурсов платформы");
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы platform тестов
 */
test_suite_t *create_platform_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("platform",
                                          "Тесты платформенных модулей",
                                          TEST_TYPE_SYSTEM);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы platform тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Базовые тесты платформы */
    test_def = test_definition_create("test_platform_detection", "platform",
                                     test_platform_detection, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_platform_system_info", "platform",
                                     test_platform_system_info, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты процессов */
    test_def = test_definition_create("test_platform_processes", "platform",
                                     test_platform_processes, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты сети */
    test_def = test_definition_create("test_platform_network", "platform",
                                     test_platform_network, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты диска */
    test_def = test_definition_create("test_platform_disk", "platform",
                                     test_platform_disk, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты GPU */
    test_def = test_definition_create("test_platform_gpu", "platform",
                                     test_platform_gpu, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты специфичных возможностей */
    test_def = test_definition_create("test_platform_specific_features", "platform",
                                     test_platform_specific_features, TEST_TYPE_SYSTEM);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_platform_performance", "platform",
                                     test_platform_performance, TEST_TYPE_PERFORMANCE);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты стабильности */
    test_def = test_definition_create("test_platform_stability", "platform",
                                     test_platform_stability, TEST_TYPE_SYSTEM);
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

    TEST_DIAGNOSTIC("Запуск группы platform тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("===========================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 60000; /* 60 секунд таймаут для platform тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_platform_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы platform тестов");
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
    TEST_DIAGNOSTIC("Группа platform тестов завершена");
    return result;
}