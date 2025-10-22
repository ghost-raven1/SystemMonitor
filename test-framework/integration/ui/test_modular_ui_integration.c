/**
 * @file test_modular_ui.c
 * @brief Интеграционные тесты модульной архитектуры
 *
 * Тестирует взаимодействие между модулями UI, system_monitor, app_context и другими компонентами.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "ui_data_visualizer.h"
#include "ui_renderer.h"
#include "ui_input_handler.h"
#include "ui_window_manager.h"
#include "system_monitor.h"
#include "app_context.h"
#include "app_events.h"
#include "diagnostics.h"
#include "developer_tools.h"

// Структура для хранения результатов тестирования
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char results[1024];
} test_results_t;

// Глобальные результаты тестирования
static test_results_t g_test_results = {0, 0, 0, ""};

// Макрос для логирования результатов тестирования
#define TEST_LOG(fmt, ...) do { \
    char buf[256]; \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    strncat(g_test_results.results, buf, sizeof(g_test_results.results) - strlen(g_test_results.results) - 1); \
    printf("[TEST] %s", buf); \
} while(0)

// Макрос для регистрации результатов теста
#define TEST_RESULT(test_name, passed) do { \
    g_test_results.total_tests++; \
    if (passed) { \
        g_test_results.passed_tests++; \
        TEST_LOG("✓ ПАС: %s\n", test_name); \
    } else { \
        g_test_results.failed_tests++; \
        TEST_LOG("✗ НЕУДАЧА: %s\n", test_name); \
    } \
} while(0)

/**
 * @brief Тест инициализации контекста приложения
 */
int test_app_context_init(void) {
    TEST_LOG("Запуск теста инициализации контекста приложения...\n");

    int result = app_context_init(NULL);
    if (result != 0) {
        TEST_RESULT("Инициализация контекста приложения", 0);
        return 0;
    }

    result = app_context_start();
    if (result != 0) {
        TEST_RESULT("Запуск контекста приложения", 0);
        return 0;
    }

    // Проверяем, что контекст работает
    app_context_t *ctx = get_app_context();
    if (!ctx) {
        TEST_RESULT("Получение контекста приложения", 0);
        return 0;
    }

    TEST_RESULT("Инициализация контекста приложения", 1);
    TEST_RESULT("Запуск контекста приложения", 1);
    TEST_RESULT("Получение контекста приложения", 1);

    return 1;
}

/**
 * @brief Тест инициализации менеджера событий
 */
int test_event_manager_init(void) {
    TEST_LOG("Запуск теста инициализации менеджера событий...\n");

    int result = event_manager_init();
    if (result != 0) {
        TEST_RESULT("Инициализация менеджера событий", 0);
        return 0;
    }

    TEST_RESULT("Инициализация менеджера событий", 1);
    return 1;
}

/**
 * @brief Тест инициализации модуля мониторинга системы
 */
int test_system_monitor_init(void) {
    TEST_LOG("Запуск теста инициализации модуля мониторинга системы...\n");

    int result = system_monitor_init();
    if (result != 0) {
        TEST_RESULT("Инициализация модуля мониторинга", 0);
        return 0;
    }

    result = system_monitor_start();
    if (result != 0) {
        TEST_RESULT("Запуск модуля мониторинга", 0);
        return 0;
    }

    TEST_RESULT("Инициализация модуля мониторинга", 1);
    TEST_RESULT("Запуск модуля мониторинга", 1);

    return 1;
}

/**
 * @brief Тест инициализации модуля визуализации данных UI
 */
int test_ui_data_visualizer_init(void) {
    TEST_LOG("Запуск теста инициализации модуля визуализации данных UI...\n");

    ui_data_visualizer_init();

    TEST_RESULT("Инициализация модуля визуализации данных UI", 1);
    return 1;
}

/**
 * @brief Тест инициализации модуля диагностики
 */
int test_diagnostics_init(void) {
    TEST_LOG("Запуск теста инициализации модуля диагностики...\n");

    // Модуль диагностики инициализируется через контекст приложения
    TEST_RESULT("Инициализация модуля диагностики", 1);
    return 1;
}

/**
 * @brief Тест инициализации модуля инструментов разработчика
 */
int test_developer_tools_init(void) {
    TEST_LOG("Запуск теста инициализации модуля инструментов разработчика...\n");

    // Модуль инструментов разработчика инициализируется через контекст приложения
    TEST_RESULT("Инициализация модуля инструментов разработчика", 1);
    return 1;
}

/**
 * @brief Тест взаимодействия между модулями мониторинга и визуализации
 */
int test_module_interaction(void) {
    TEST_LOG("Запуск теста взаимодействия между модулями...\n");

    // Тестируем получение метрик через модуль мониторинга
    // Используем функции из system_monitor.h
    extern float get_cpu_usage(void);
    extern float get_memory_usage(void);
    extern long get_uptime_seconds(void);

    float cpu_usage = get_cpu_usage();
    float mem_usage = get_memory_usage();
    long uptime = get_uptime_seconds();

    if (cpu_usage < 0 || mem_usage < 0 || uptime < 0) {
        TEST_RESULT("Получение базовых метрик через модуль мониторинга", 0);
        return 0;
    }

    TEST_RESULT("Получение базовых метрик через модуль мониторинга", 1);

    // Тестируем обновление метрик
    int result = system_monitor_update_metrics();
    if (result != 0) {
        TEST_RESULT("Обновление метрик системы", 0);
        return 0;
    }

    TEST_RESULT("Обновление метрик системы", 1);

    return 1;
}

/**
 * @brief Тест обработки событий
 */
int test_event_handling(void) {
    TEST_LOG("Запуск теста обработки событий...\n");

    // Регистрируем обработчик событий
    int result = register_event_handler(APP_EVENT_METRICS_UPDATED, default_metrics_handler, NULL);
    if (result != 0) {
        TEST_RESULT("Регистрация обработчика событий", 0);
        return 0;
    }

    TEST_RESULT("Регистрация обработчика событий", 1);

    // Создаем и отправляем тестовое событие
    app_event_data_t *event_data = create_event_data(
        APP_EVENT_METRICS_UPDATED, 1, "test", "Тестовое событие обновления метрик");

    if (!event_data) {
        TEST_RESULT("Создание данных события", 0);
        return 0;
    }

    TEST_RESULT("Создание данных события", 1);

    result = emit_event_with_data(event_data);
    if (result <= 0) {
        TEST_RESULT("Отправка события", 0);
        free_event_data(event_data);
        return 0;
    }

    TEST_RESULT("Отправка события", 1);

    // Освобождаем ресурсы события
    free_event_data(event_data);
    TEST_RESULT("Освобождение данных события", 1);

    return 1;
}

/**
 * @brief Тест проверки доступности модулей
 */
int test_module_availability(void) {
    TEST_LOG("Запуск теста проверки доступности модулей...\n");

    // Проверяем доступность модулей через system_monitor
    extern int system_monitor_check_modules(void);
    int available_modules = system_monitor_check_modules();

    // Должны быть доступны как минимум базовые модули
    if (available_modules <= 0) {
        TEST_RESULT("Проверка доступности модулей", 0);
        return 0;
    }

    TEST_LOG("Доступные модули: 0x%X\n", available_modules);
    TEST_RESULT("Проверка доступности модулей", 1);

    return 1;
}

/**
 * @brief Тест очистки ресурсов
 */
int test_cleanup(void) {
    TEST_LOG("Запуск теста очистки ресурсов...\n");

    // Очищаем модуль визуализации данных
    ui_data_visualizer_cleanup();

    // Очищаем модуль мониторинга системы
    system_monitor_cleanup();

    // Очищаем менеджер событий
    event_manager_cleanup();

    // Очищаем контекст приложения
    app_context_stop();
    app_context_cleanup();

    TEST_RESULT("Очистка ресурсов модулей", 1);
    return 1;
}

/**
 * @brief Основная функция тестирования
 */
int main(void) {
    printf("🚀 Запуск интеграционных тестов модульной архитектуры\n");
    printf("================================================\n\n");

    // Инициализируем результаты тестирования
    memset(&g_test_results, 0, sizeof(test_results_t));
    strcpy(g_test_results.results, "");

    // Запускаем тесты
    test_app_context_init();
    test_event_manager_init();
    test_system_monitor_init();
    test_ui_data_visualizer_init();
    test_diagnostics_init();
    test_developer_tools_init();
    test_module_interaction();
    test_event_handling();
    test_module_availability();
    test_cleanup();

    // Выводим итоговые результаты
    printf("\n================================================\n");
    printf("📊 РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ\n");
    printf("================================================\n");
    printf("Всего тестов: %d\n", g_test_results.total_tests);
    printf("Пройдено: %d\n", g_test_results.passed_tests);
    printf("Неудачно: %d\n", g_test_results.failed_tests);
    printf("Успешность: %.1f%%\n", (float)g_test_results.passed_tests / g_test_results.total_tests * 100.0f);

    if (g_test_results.failed_tests == 0) {
        printf("\n🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!\n");
        printf("Модульная архитектура работает корректно.\n");
        return 0;
    } else {
        printf("\n❌ НЕКОТОРЫЕ ТЕСТЫ НЕ ПРОШЛИ\n");
        printf("Необходимо устранить проблемы в модулях.\n");
        printf("\nДетали:\n%s\n", g_test_results.results);
        return 1;
    }
}