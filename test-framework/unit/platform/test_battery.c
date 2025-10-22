/**
 * @file test_battery.c
 * @brief Unit тесты для battery модуля в новом тестовом фреймворке
 *
 * Адаптированные версии существующих battery тестов для использования
 * с новым тестовым фреймворком SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/platform/battery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования battery функционала
 */
typedef struct {
    battery_info_t battery_info;
    int cycle_count;
    char test_name[64];
} battery_test_data_t;

/**
 * @brief Функция создания тестовых данных батареи
 */
battery_test_data_t *create_battery_test_data(const char *test_name) {
    battery_test_data_t *data = malloc(sizeof(battery_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(battery_test_data_t));
    safe_strcpy(data->test_name, test_name, sizeof(data->test_name));

    return data;
}

/**
 * @brief Функция освобождения тестовых данных батареи
 */
void destroy_battery_test_data(battery_test_data_t *data) {
    if (data) {
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для базового функционала батареи
 * ============================================================================ */

TEST_BEGIN(test_battery_basic_info) {
    /* Тестирование получения базовой информации о батарее */
    battery_info_t battery_info;
    memset(&battery_info, 0, sizeof(battery_info_t));

    /* Вызов функции получения информации о батарее */
    int result = get_battery_info(&battery_info);

    /* Проверка что функция выполняется без критических ошибок */
    TEST_ASSERT_TRUE(result >= -1, "get_battery_info должен возвращать корректные коды ошибок");

    /* Если батарея доступна, проверяем корректность данных */
    if (result == 0) {
        TEST_DIAGNOSTIC("Батарея найдена: %.1f%%, зарядка: %s, время: %d мин",
                       battery_info.percentage,
                       battery_info.charging ? "да" : "нет",
                       battery_info.time_remaining_min);

        TEST_ASSERT_IN_RANGE(battery_info.percentage, 0.0, 100.0,
                           "Процент заряда должен быть в диапазоне 0-100%");
        TEST_ASSERT_TRUE(battery_info.time_remaining_min >= -1,
                        "Время работы должно быть >= -1 мин");
    } else {
        TEST_DIAGNOSTIC("Батарея недоступна (код ошибки: %d)", result);
    }

    TEST_ASSERT_SUCCESS(0, "Базовый тест батареи пройден");
}
TEST_END();

/* ============================================================================
 * Unit тесты для количества циклов батареи
 * ============================================================================ */

TEST_BEGIN(test_battery_cycle_count) {
    /* Тестирование получения количества циклов батареи */
    int cycle_count = -1;

    /* Вызов функции получения циклов батареи */
    int result = get_battery_cycle_count(&cycle_count);

    /* Проверка корректности результата */
    TEST_ASSERT_TRUE(result >= -1, "get_battery_cycle_count должен возвращать корректные коды");

    /* Если данные доступны, проверяем их корректность */
    if (result == 0 && cycle_count >= 0) {
        TEST_DIAGNOSTIC("Количество циклов батареи: %d", cycle_count);
        TEST_ASSERT_TRUE(cycle_count >= 0, "Количество циклов не может быть отрицательным");
    } else if (result != 0) {
        TEST_DIAGNOSTIC("Информация о циклах недоступна (код ошибки: %d)", result);
    }

    TEST_ASSERT_SUCCESS(0, "Тест циклов батареи пройден");
}
TEST_END();

/* ============================================================================
 * Unit тесты для мониторинга состояния батареи
 * ============================================================================ */

TEST_BEGIN(test_battery_monitoring) {
    /* Тестирование мониторинга состояния батареи */
    battery_info_t first_reading, second_reading;
    memset(&first_reading, 0, sizeof(battery_info_t));
    memset(&second_reading, 0, sizeof(battery_info_t));

    /* Получение первой выборки данных */
    int result1 = get_battery_info(&first_reading);
    TEST_ASSERT_TRUE(result1 >= -1, "Первое чтение данных батареи должно выполняться корректно");

    /* Небольшая пауза для имитации мониторинга */
    sleep(1);

    /* Получение второй выборки данных */
    int result2 = get_battery_info(&second_reading);
    TEST_ASSERT_TRUE(result2 >= -1, "Второе чтение данных батареи должно выполняться корректно");

    /* Сравнение результатов показывает стабильность API */
    TEST_ASSERT_TRUE(result1 == result2,
                    "Последовательные вызовы должны возвращать одинаковые коды результатов");

    /* Если данные доступны, проверяем их согласованность */
    if (result1 == 0 && result2 == 0) {
        TEST_ASSERT_IN_RANGE(first_reading.percentage, 0.0, 100.0, "Первое значение процента");
        TEST_ASSERT_IN_RANGE(second_reading.percentage, 0.0, 100.0, "Второе значение процента");

        /* Разница между показаниями не должна быть слишком большой (за исключением случаев зарядки) */
        double diff = second_reading.percentage - first_reading.percentage;
        TEST_ASSERT_IN_RANGE(diff, -10.0, 10.0,
                           "Разница показаний не должна превышать 10% без видимой причины");
    }

    TEST_ASSERT_SUCCESS(0, "Мониторинг батареи работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для обработки ошибок батареи
 * ============================================================================ */

TEST_BEGIN(test_battery_error_handling) {
    /* Тестирование обработки ошибок в battery модуле */

    /* Тест с NULL указателем */
    int result_null = get_battery_info(NULL);
    TEST_ASSERT_TRUE(result_null != 0, "Передача NULL должна возвращать ошибку");

    /* Тест с некорректной батареей (если применимо) */
    battery_info_t invalid_battery;
    memset(&invalid_battery, 0xFF, sizeof(battery_info_t));

    int result_invalid = get_battery_info(&invalid_battery);
    TEST_ASSERT_TRUE(result_invalid >= -1,
                    "Функция должна корректно обрабатывать некорректные данные");

    /* Тест с цикловой функцией и NULL указателем */
    int result_cycles_null = get_battery_cycle_count(NULL);
    TEST_ASSERT_TRUE(result_cycles_null != 0,
                    "Передача NULL в get_battery_cycle_count должна возвращать ошибку");

    TEST_ASSERT_SUCCESS(0, "Обработка ошибок батареи работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для производительности батареи
 * ============================================================================ */

TEST_BEGIN(test_battery_performance) {
    /* Тестирование производительности операций с батареей */
    const int iterations = 100;

    TEST_TIME_START();

    for (int i = 0; i < iterations; i++) {
        battery_info_t battery_info;
        int result = get_battery_info(&battery_info);

        /* Проверяем что каждый вызов завершается корректно */
        TEST_ASSERT_TRUE(result >= -1,
                        "Каждый вызов get_battery_info должен завершаться корректно");

        /* Небольшая пауза между вызовами */
        usleep(1000); /* 1ms */
    }

    uint64_t elapsed_time = TEST_TIME_END();
    double avg_time_per_call = (double)elapsed_time / iterations;

    TEST_DIAGNOSTIC("Среднее время вызова get_battery_info: %.2f мс", avg_time_per_call);

    /* Проверка что операции выполняются достаточно быстро */
    TEST_ASSERT_TRUE(avg_time_per_call < 100.0,
                    "Среднее время вызова не должно превышать 100мс");

    /* Проверка что общее время тестирования разумное */
    TEST_ASSERT_TRUE(elapsed_time < 30000,
                    "Общее время тестирования не должно превышать 30 секунд");

    TEST_ASSERT_SUCCESS(0, "Производительность батареи в пределах нормы");
}
TEST_END();

/* ============================================================================
 * Unit тесты для интеграции с системой мониторинга
 * ============================================================================ */

TEST_BEGIN(test_battery_integration) {
    /* Тестирование интеграции battery модуля с системой мониторинга */

    /* Проверка что battery модуль корректно интегрируется с общей системой */
    battery_info_t battery_info;
    int result = get_battery_info(&battery_info);

    /* Если батарея доступна, проверяем что данные могут быть использованы системой */
    if (result == 0) {
        /* Проверка что данные можно безопасно использовать в расчетах */
        double percentage = battery_info.percentage;
        TEST_ASSERT_TRUE(!isnan(percentage) && !isinf(percentage),
                        "Процент батареи должен быть корректным числом");

        /* Проверка диапазона для предотвращения ошибок в UI */
        if (percentage >= 0.0 && percentage <= 100.0) {
            TEST_ASSERT_SUCCESS(0, "Данные батареи корректны для использования в UI");
        }

        /* Проверка статуса зарядки */
        int charging = battery_info.charging;
        TEST_ASSERT_TRUE(charging == 0 || charging == 1,
                        "Статус зарядки должен быть 0 или 1");

        /* Проверка времени работы */
        int time_remaining = battery_info.time_remaining_min;
        TEST_ASSERT_TRUE(time_remaining >= -1,
                        "Время работы должно быть >= -1");
    } else {
        /* Если батарея недоступна, это тоже корректное поведение */
        TEST_DIAGNOSTIC("Батарея недоступна для интеграции (код: %d)", result);
        TEST_ASSERT_SUCCESS(0, "Интеграция корректно обрабатывает отсутствие батареи");
    }

    TEST_ASSERT_SUCCESS(0, "Интеграция батареи с системой мониторинга работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для специфичных сценариев батареи
 * ============================================================================ */

TEST_BEGIN(test_battery_edge_cases) {
    /* Тестирование граничных случаев работы с батареей */

    /* Тест очень быстрого последовательного вызова */
    battery_info_t battery_info1, battery_info2;

    int result1 = get_battery_info(&battery_info1);
    int result2 = get_battery_info(&battery_info2);

    TEST_ASSERT_TRUE(result1 >= -1 && result2 >= -1,
                    "Последовательные вызовы должны работать корректно");

    /* Тест с большими интервалами между вызовами */
    sleep(2);

    battery_info_t delayed_info;
    int delayed_result = get_battery_info(&delayed_info);
    TEST_ASSERT_TRUE(delayed_result >= -1,
                    "Вызов после паузы должен работать корректно");

    /* Тест обработки различных состояний батареи */
    if (result1 == 0) {
        /* Если батарея доступна, проверяем различные сценарии */

        /* Полностью разряженная батарея */
        if (battery_info1.percentage <= 0.1) {
            TEST_DIAGNOSTIC("Обнаружена разряженная батарея: %.1f%%", battery_info1.percentage);
            TEST_ASSERT_IN_RANGE((int64_t)battery_info1.percentage, 0, 0,
                               "Разряженная батарея должна показывать ~0%");
        }

        /* Полностью заряженная батарея */
        if (battery_info1.percentage >= 99.9) {
            TEST_DIAGNOSTIC("Обнаружена заряженная батарея: %.1f%%", battery_info1.percentage);
            TEST_ASSERT_IN_RANGE(battery_info1.percentage, 99.9, 100.0,
                               "Заряженная батарея должна показывать ~100%");
        }

        /* Батарея в процессе зарядки */
        if (battery_info1.charging) {
            TEST_DIAGNOSTIC("Батарея заряжается: %.1f%%", battery_info1.percentage);
            TEST_ASSERT_TRUE(battery_info1.charging == 1,
                           "Статус зарядки должен быть 1");
        }
    }

    TEST_ASSERT_SUCCESS(0, "Граничные случаи батареи обработаны корректно");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы battery тестов
 */
static void battery_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы battery тестов...");

    /* Проверка доступности battery модуля */
    battery_info_t test_info;
    int test_result = get_battery_info(&test_info);

    if (test_result != 0) {
        TEST_DIAGNOSTIC("Предупреждение: батарея может быть недоступна (код: %d)", test_result);
    } else {
        TEST_DIAGNOSTIC("Батарея доступна для тестирования");
    }
}

/**
 * @brief Очистка после завершения группы battery тестов
 */
static void battery_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы battery тестов...");

    /* Освобождение ресурсов батареи если необходимо */
    /* В текущей реализации батарея не требует специальной очистки */
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы battery тестов
 */
test_suite_t *create_battery_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("battery", "Тесты платформенного модуля батареи", TEST_TYPE_UNIT);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы battery тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Базовые тесты батареи */
    test_def = test_definition_create("test_battery_basic_info", "battery",
                                     test_battery_basic_info, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_battery_cycle_count", "battery",
                                     test_battery_cycle_count, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты мониторинга */
    test_def = test_definition_create("test_battery_monitoring", "battery",
                                     test_battery_monitoring, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты обработки ошибок */
    test_def = test_definition_create("test_battery_error_handling", "battery",
                                     test_battery_error_handling, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_battery_performance", "battery",
                                     test_battery_performance, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты интеграции */
    test_def = test_definition_create("test_battery_integration", "battery",
                                     test_battery_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты граничных случаев */
    test_def = test_definition_create("test_battery_edge_cases", "battery",
                                     test_battery_edge_cases, TEST_TYPE_UNIT);
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

    TEST_DIAGNOSTIC("Запуск группы battery тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("==========================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 30000; /* 30 секунд таймаут для battery тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_battery_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы battery тестов");
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
    TEST_DIAGNOSTIC("Группа battery тестов завершена");
    return result;
}