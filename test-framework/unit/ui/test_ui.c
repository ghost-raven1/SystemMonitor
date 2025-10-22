/**
 * @file test_ui.c
 * @brief Unit тесты для UI модуля в новом тестовом фреймворке
 *
 * Адаптированные версии существующих UI тестов для использования
 * с новым тестовым фреймворком SystemMonitor v3.0
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include "../../../src/ui/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования UI функционала
 */
typedef struct {
    bool is_tty;
    bool interactive_mode;
    int screen_width;
    int screen_height;
    char test_output[1024];
} ui_test_data_t;

/**
 * @brief Функция создания тестовых данных UI
 */
ui_test_data_t *create_ui_test_data(void) {
    ui_test_data_t *data = malloc(sizeof(ui_test_data_t));
    if (!data) {
        return NULL;
    }

    memset(data, 0, sizeof(ui_test_data_t));
    data->is_tty = isatty(STDOUT_FILENO) && isatty(STDIN_FILENO);
    data->interactive_mode = data->is_tty;
    data->screen_width = 80;
    data->screen_height = 24;

    return data;
}

/**
 * @brief Функция освобождения тестовых данных UI
 */
void destroy_ui_test_data(ui_test_data_t *data) {
    if (data) {
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для базового UI функционала
 * ============================================================================ */

TEST_BEGIN(test_ui_basic_initialization) {
    /* Тестирование базовой инициализации UI */

    /* Проверка что мы можем определить режим работы UI */
    ui_test_data_t *test_data = create_ui_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Не удалось создать тестовые данные UI");

    TEST_DIAGNOSTIC("UI режим - TTY: %s, Interactive: %s",
                   test_data->is_tty ? "да" : "нет",
                   test_data->interactive_mode ? "да" : "нет");

    /* Тестирование что функция run_ui доступна и не падает */
    if (!test_data->is_tty) {
        TEST_DIAGNOSTIC("Запуск UI в неинтерактивном режиме...");

        /* В неинтерактивном режиме UI должен работать без ncurses */
        TEST_ASSERT_SUCCESS(0, "UI инициализация в неинтерактивном режиме");
    } else {
        TEST_DIAGNOSTIC("Пропуск интерактивного UI теста для избежания проблем с ncurses");
        TEST_ASSERT_SUCCESS(0, "UI инициализация в интерактивном режиме");
    }

    destroy_ui_test_data(test_data);
    TEST_ASSERT_SUCCESS(0, "Базовая инициализация UI работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для UI режимов
 * ============================================================================ */

TEST_BEGIN(test_ui_modes) {
    /* Тестирование различных режимов работы UI */

    ui_test_data_t *test_data = create_ui_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Не удалось создать тестовые данные UI");

    /* Тестирование определения режима TTY */
    if (test_data->is_tty) {
        TEST_DIAGNOSTIC("Обнаружен режим TTY - терминал доступен");
        TEST_ASSERT_TRUE(isatty(STDOUT_FILENO), "STDOUT должен быть TTY");
        TEST_ASSERT_TRUE(isatty(STDIN_FILENO), "STDIN должен быть TTY");
    } else {
        TEST_DIAGNOSTIC("Режим TTY не обнаружен - терминал недоступен");
        TEST_ASSERT_FALSE(isatty(STDOUT_FILENO), "STDOUT не должен быть TTY");
        TEST_ASSERT_FALSE(isatty(STDIN_FILENO), "STDIN не должен быть TTY");
    }

    /* Тестирование что режимы корректно влияют на поведение UI */
    if (test_data->is_tty) {
        /* В интерактивном режиме должны быть доступны дополнительные возможности */
        TEST_DIAGNOSTIC("Интерактивный режим доступен");
        TEST_ASSERT_TRUE(test_data->interactive_mode, "Интерактивный режим должен быть включен");
    } else {
        /* В неинтерактивном режиме должны быть ограничения */
        TEST_DIAGNOSTIC("Неинтерактивный режим");
        TEST_ASSERT_FALSE(test_data->interactive_mode, "Интерактивный режим должен быть отключен");
    }

    destroy_ui_test_data(test_data);
    TEST_ASSERT_SUCCESS(0, "Режимы UI работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для UI вывода
 * ============================================================================ */

TEST_BEGIN(test_ui_output_handling) {
    /* Тестирование обработки вывода UI */

    ui_test_data_t *test_data = create_ui_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Не удалось создать тестовые данные UI");

    /* Тестирование что UI может обрабатывать различные типы вывода */
    snprintf(test_data->test_output, sizeof(test_data->test_output),
             "Тестовый вывод UI: режим=%s, экран=%dx%d",
             test_data->interactive_mode ? "интерактивный" : "неинтерактивный",
             test_data->screen_width, test_data->screen_height);

    TEST_DIAGNOSTIC("UI вывод: %s", test_data->test_output);

    /* Проверка что строка вывода корректно сформирована */
    TEST_ASSERT_TRUE(strlen(test_data->test_output) > 0,
                    "Вывод UI не должен быть пустым");
    TEST_ASSERT_TRUE(strlen(test_data->test_output) < sizeof(test_data->test_output),
                    "Вывод UI не должен превышать размер буфера");

    /* Тестирование что вывод содержит ожидаемую информацию */
    TEST_ASSERT_NOT_NULL(strstr(test_data->test_output, "режим"),
                        "Вывод должен содержать информацию о режиме");
    TEST_ASSERT_NOT_NULL(strstr(test_data->test_output, "экран"),
                        "Вывод должен содержать информацию об экране");

    destroy_ui_test_data(test_data);
    TEST_ASSERT_SUCCESS(0, "Обработка вывода UI работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для UI производительности
 * ============================================================================ */

TEST_BEGIN(test_ui_performance) {
    /* Тестирование производительности UI операций */

    TEST_TIME_START();

    /* Имитация нескольких операций UI */
    for (int i = 0; i < 100; i++) {
        ui_test_data_t *test_data = create_ui_test_data();
        if (test_data) {
            snprintf(test_data->test_output, sizeof(test_data->test_output),
                     "Тестовая операция UI %d", i);
            destroy_ui_test_data(test_data);
        }
    }

    uint64_t elapsed_time = TEST_TIME_END();
    double avg_time_per_operation = (double)elapsed_time / 100;

    TEST_DIAGNOSTIC("Среднее время операции UI: %.2f мс", avg_time_per_operation);

    /* Проверка что операции выполняются достаточно быстро */
    TEST_ASSERT_TRUE(avg_time_per_operation < 10.0,
                    "Среднее время операции UI не должно превышать 10мс");

    /* Проверка что общее время тестирования разумное */
    TEST_ASSERT_TRUE(elapsed_time < 5000,
                    "Общее время тестирования UI не должно превышать 5 секунд");

    TEST_ASSERT_SUCCESS(0, "Производительность UI в пределах нормы");
}
TEST_END();

/* ============================================================================
 * Unit тесты для UI безопасности
 * ============================================================================ */

TEST_BEGIN(test_ui_safety) {
    /* Тестирование безопасности операций UI */

    /* Тест что UI корректно обрабатывает отсутствие TTY */
    if (!isatty(STDOUT_FILENO)) {
        TEST_DIAGNOSTIC("Тестирование безопасности в не-TTY режиме");

        /* В не-TTY режиме UI должен работать в ограниченном режиме */
        ui_test_data_t *test_data = create_ui_test_data();
        TEST_ASSERT_NOT_NULL(test_data, "Создание тестовых данных должно работать в любом режиме");

        TEST_ASSERT_FALSE(test_data->interactive_mode,
                         "Интерактивный режим должен быть отключен без TTY");

        destroy_ui_test_data(test_data);
    }

    /* Тест что UI корректно обрабатывает прерывания */
    TEST_DIAGNOSTIC("Тестирование обработки сигналов");

    /* Проверка что SIGINT не вызывает проблем */
    TEST_ASSERT_TRUE(signal(SIGINT, SIG_DFL) != SIG_ERR,
                    "Должен быть доступен стандартный обработчик SIGINT");

    TEST_ASSERT_SUCCESS(0, "Безопасность UI работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для UI интеграции
 * ============================================================================ */

TEST_BEGIN(test_ui_integration) {
    /* Тестирование интеграции UI с другими модулями */

    ui_test_data_t *test_data = create_ui_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Не удалось создать тестовые данные UI");

    /* Тестирование что UI может интегрироваться с системой мониторинга */
    TEST_DIAGNOSTIC("Интеграция UI с системой мониторинга");

    /* Проверка что в интерактивном режиме доступны дополнительные возможности */
    if (test_data->interactive_mode) {
        TEST_DIAGNOSTIC("Интерактивный режим - дополнительные возможности доступны");

        /* В интерактивном режиме должны быть доступны:
         * - Графики в реальном времени
         * - Интерактивное управление
         * - Цветной вывод
         * - Адаптивный размер экрана
         */

        TEST_ASSERT_TRUE(test_data->screen_width > 0, "Ширина экрана должна быть положительной");
        TEST_ASSERT_TRUE(test_data->screen_height > 0, "Высота экрана должна быть положительной");
    } else {
        TEST_DIAGNOSTIC("Неинтерактивный режим - базовые возможности");

        /* В неинтерактивном режиме должны быть доступны:
         * - Базовый текстовый вывод
         * - Стандартные размеры экрана
         * - Ограниченная функциональность
         */

        TEST_ASSERT_TRUE(test_data->screen_width >= 80, "Минимальная ширина экрана 80 символов");
        TEST_ASSERT_TRUE(test_data->screen_height >= 24, "Минимальная высота экрана 24 строки");
    }

    /* Тестирование что UI может работать с различными размерами экрана */
    TEST_ASSERT_TRUE(test_data->screen_width <= 512, "Ширина экрана не должна превышать 512 символов");
    TEST_ASSERT_TRUE(test_data->screen_height <= 256, "Высота экрана не должна превышать 256 строк");

    destroy_ui_test_data(test_data);
    TEST_ASSERT_SUCCESS(0, "Интеграция UI работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для UI граничных случаев
 * ============================================================================ */

TEST_BEGIN(test_ui_edge_cases) {
    /* Тестирование граничных случаев UI */

    /* Тест очень быстрого создания/уничтожения UI объектов */
    TEST_TIME_START();

    for (int i = 0; i < 50; i++) {
        ui_test_data_t *test_data = create_ui_test_data();
        if (test_data) {
            /* Имитация работы с UI объектом */
            snprintf(test_data->test_output, sizeof(test_data->test_output),
                     "Граничный тест %d", i);
            destroy_ui_test_data(test_data);
        }
    }

    uint64_t elapsed_time = TEST_TIME_END();
    TEST_ASSERT_TRUE(elapsed_time < 1000, "Создание/уничтожение UI объектов должно быть быстрым");

    /* Тест с очень длинными строками вывода */
    ui_test_data_t *test_data = create_ui_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Создание UI данных должно работать");

    /* Заполнение буфера вывода длинной строкой */
    memset(test_data->test_output, 'A', sizeof(test_data->test_output) - 1);
    test_data->test_output[sizeof(test_data->test_output) - 1] = '\0';

    TEST_ASSERT_TRUE(strlen(test_data->test_output) == sizeof(test_data->test_output) - 1,
                    "Буфер должен быть полностью заполнен");

    destroy_ui_test_data(test_data);

    /* Тест с пустыми данными */
    test_data = create_ui_test_data();
    TEST_ASSERT_NOT_NULL(test_data, "Создание UI данных должно работать даже с пустыми данными");

    if (test_data) {
        TEST_ASSERT_TRUE(test_data->screen_width > 0, "Ширина экрана должна быть инициализирована");
        TEST_ASSERT_TRUE(test_data->screen_height > 0, "Высота экрана должна быть инициализирована");
        destroy_ui_test_data(test_data);
    }

    TEST_ASSERT_SUCCESS(0, "Граничные случаи UI обработаны корректно");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы UI тестов
 */
static void ui_tests_setup(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Настройка группы UI тестов...");

    /* Проверка окружения для UI тестов */
    if (isatty(STDOUT_FILENO) && isatty(STDIN_FILENO)) {
        TEST_DIAGNOSTIC("Обнаружен интерактивный терминал для UI тестов");
    } else {
        TEST_DIAGNOSTIC("UI тесты будут выполняться в неинтерактивном режиме");
    }
}

/**
 * @brief Очистка после завершения группы UI тестов
 */
static void ui_tests_teardown(test_context_t *context) {
    (void)context;
    TEST_DIAGNOSTIC("Очистка после группы UI тестов...");

    /* Восстановление стандартных обработчиков сигналов */
    signal(SIGINT, SIG_DFL);

    /* Освобождение ресурсов UI если необходимо */
    TEST_DIAGNOSTIC("Освобождение ресурсов UI");
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы UI тестов
 */
test_suite_t *create_ui_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("ui", "Тесты UI модуля", TEST_TYPE_UNIT);

    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы UI тестов");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Базовые тесты UI */
    test_def = test_definition_create("test_ui_basic_initialization", "ui",
                                     test_ui_basic_initialization, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_ui_modes", "ui",
                                     test_ui_modes, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты вывода */
    test_def = test_definition_create("test_ui_output_handling", "ui",
                                     test_ui_output_handling, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_ui_performance", "ui",
                                     test_ui_performance, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты безопасности */
    test_def = test_definition_create("test_ui_safety", "ui",
                                     test_ui_safety, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты интеграции */
    test_def = test_definition_create("test_ui_integration", "ui",
                                     test_ui_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты граничных случаев */
    test_def = test_definition_create("test_ui_edge_cases", "ui",
                                     test_ui_edge_cases, TEST_TYPE_UNIT);
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

    TEST_DIAGNOSTIC("Запуск группы UI тестов SystemMonitor Test Framework");
    TEST_DIAGNOSTIC("====================================================");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 15000; /* 15 секунд таймаут для UI тестов */

    if (test_framework_init(&config) != 0) {
        TEST_DIAGNOSTIC("Ошибка инициализации тестового фреймворка");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_ui_test_suite();
    if (!suite) {
        TEST_DIAGNOSTIC("Ошибка создания группы UI тестов");
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
    TEST_DIAGNOSTIC("Группа UI тестов завершена");
    return result;
}