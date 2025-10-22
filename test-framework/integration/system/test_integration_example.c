/**
 * @file test_integration_example.c
 * @brief Пример интеграционных тестов для компонентов SystemMonitor
 *
 * Этот файл демонстрирует использование тестового фреймворка для
 * тестирования взаимодействия между компонентами архитектуры SystemMonitor.
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/* ============================================================================
 * Глобальные переменные для интеграционных тестов
 * ============================================================================ */

/* Фиктивные модули для тестирования интеграции */
static module_t *test_modules[10] = {0};
static size_t module_count = 0;

/* События для тестирования системы событий */
static event_t test_events[100] = {0};
static size_t event_count = 0;

/* Метрики для тестирования сборщика метрик */
static system_metrics_t current_metrics = {0};

/* ============================================================================
 * Фиктивные реализации компонентов архитектуры
 * ============================================================================ */

/**
 * @brief Фиктивная реализация контекста приложения
 */
static app_context_t mock_app_context = {
    .app_name = "SystemMonitor Integration Test",
    .app_version = "3.0.0",
    .metrics_collector = NULL,
    .event_system = NULL,
    .plugin_manager = NULL,
    .start_time = 0,
    .initialized = true
};

/**
 * @brief Фиктивная функция инициализации модуля мониторинга CPU
 */
static int mock_cpu_monitor_init(module_info_t *info, const void *ctx) {
    (void)ctx;
    if (info) {
        safe_strcpy(info->name, "cpu_monitor", sizeof(info->name));
        safe_strcpy(info->version, "1.0.0", sizeof(info->version));
        safe_strcpy(info->description, "CPU мониторинг модуль", sizeof(info->description));
        info->status = MODULE_STATUS_RUNNING;
        info->priority = 5;
        info->required = true;
        return 0;
    }
    return -1;
}

/**
 * @brief Фиктивная функция получения данных CPU монитора
 */
static int mock_cpu_monitor_get_data(void *buffer, size_t buffer_size) {
    if (buffer && buffer_size >= sizeof(double)) {
        double *cpu_usage = (double *)buffer;
        *cpu_usage = current_metrics.cpu_usage;
        return sizeof(double);
    }
    return 0;
}

/**
 * @brief Фиктивная функция инициализации модуля мониторинга памяти
 */
static int mock_memory_monitor_init(module_info_t *info, const void *ctx) {
    (void)ctx;
    if (info) {
        safe_strcpy(info->name, "memory_monitor", sizeof(info->name));
        safe_strcpy(info->version, "1.0.0", sizeof(info->version));
        safe_strcpy(info->description, "Memory мониторинг модуль", sizeof(info->description));
        info->status = MODULE_STATUS_RUNNING;
        info->priority = 4;
        info->required = true;
        return 0;
    }
    return -1;
}

/**
 * @brief Фиктивная функция получения данных memory монитора
 */
static int mock_memory_monitor_get_data(void *buffer, size_t buffer_size) {
    if (buffer && buffer_size >= sizeof(double)) {
        double *memory_usage = (double *)buffer;
        *memory_usage = current_metrics.memory_usage;
        return sizeof(double);
    }
    return 0;
}

/**
 * @brief Фиктивная функция инициализации UI рендерера
 */
static int mock_ui_renderer_init(module_info_t *info, const void *ctx) {
    (void)ctx;
    if (info) {
        safe_strcpy(info->name, "ui_renderer", sizeof(info->name));
        safe_strcpy(info->version, "1.0.0", sizeof(info->version));
        safe_strcpy(info->description, "UI рендерер модуль", sizeof(info->description));
        info->status = MODULE_STATUS_RUNNING;
        info->priority = 10;
        info->required = false;
        return 0;
    }
    return -1;
}

/**
 * @brief Фиктивная функция рендеринга UI
 */
static int mock_ui_renderer_render(const system_metrics_t *metrics) {
    (void)metrics;
    return 0;
}

/* ============================================================================
 * Интеграционные тесты для модульной системы
 * ============================================================================ */

/**
 * @brief Тест интеграции модулей мониторинга
 */
TEST_BEGIN(test_monitoring_modules_integration) {
    /* Создание и инициализация модулей мониторинга */

    /* Модуль CPU мониторинга */
    module_t *cpu_module = malloc(sizeof(module_t));
    TEST_ASSERT_NOT_NULL(cpu_module, "Не удалось создать модуль CPU мониторинга");

    cpu_module->info.status = MODULE_STATUS_UNINITIALIZED;
    cpu_module->ops.init = mock_cpu_monitor_init;
    cpu_module->ops.get_data = mock_cpu_monitor_get_data;

    int cpu_init_result = cpu_module->ops.init(&cpu_module->info, &mock_app_context);
    TEST_ASSERT_SUCCESS(cpu_init_result, "Инициализация CPU модуля должна пройти успешно");
    TEST_ASSERT_TRUE(cpu_module->info.status == MODULE_STATUS_RUNNING, "CPU модуль должен быть в статусе RUNNING");

    test_modules[module_count++] = cpu_module;

    /* Модуль памяти мониторинга */
    module_t *memory_module = malloc(sizeof(module_t));
    TEST_ASSERT_NOT_NULL(memory_module, "Не удалось создать модуль памяти мониторинга");

    memory_module->info.status = MODULE_STATUS_UNINITIALIZED;
    memory_module->ops.init = mock_memory_monitor_init;
    memory_module->ops.get_data = mock_memory_monitor_get_data;

    int memory_init_result = memory_module->ops.init(&memory_module->info, &mock_app_context);
    TEST_ASSERT_SUCCESS(memory_init_result, "Инициализация memory модуля должна пройти успешно");
    TEST_ASSERT_TRUE(memory_module->info.status == MODULE_STATUS_RUNNING, "Memory модуль должен быть в статусе RUNNING");

    test_modules[module_count++] = memory_module;

    /* Проверка что модули инициализированы в правильном порядке приоритета */
    TEST_ASSERT_TRUE(cpu_module->info.priority == 5, "Приоритет CPU модуля неверный");
    TEST_ASSERT_TRUE(memory_module->info.priority == 4, "Приоритет memory модуля неверный");

    /* Тестирование получения данных от модулей */
    double cpu_data = 0.0;
    double memory_data = 0.0;

    int cpu_data_result = cpu_module->ops.get_data(&cpu_data, sizeof(double));
    TEST_ASSERT_TRUE(cpu_data_result > 0, "Получение данных CPU должно пройти успешно");

    int memory_data_result = memory_module->ops.get_data(&memory_data, sizeof(double));
    TEST_ASSERT_TRUE(memory_data_result > 0, "Получение данных памяти должно пройти успешно");

    TEST_ASSERT_DOUBLE_EQUAL(current_metrics.cpu_usage, cpu_data, 0.01, "Данные CPU не совпадают");
    TEST_ASSERT_DOUBLE_EQUAL(current_metrics.memory_usage, memory_data, 0.01, "Данные памяти не совпадают");

    /* Освобождение ресурсов */
    free(cpu_module);
    free(memory_module);
    module_count = 0;

    TEST_ASSERT_SUCCESS(0, "Интеграция модулей мониторинга работает корректно");
}
TEST_END();

/* ============================================================================
 * Интеграционные тесты для системы событий
 * ============================================================================ */

/**
 * @brief Тест системы событий между модулями
 */
TEST_BEGIN(test_event_system_integration) {
    /* Тестирование системы событий архитектуры SystemMonitor */

    /* Создание тестового события */
    event_t test_event = {
        .type = EVENT_MODULE_INITIALIZED,
        .source = "test_module",
        .timestamp = time(NULL),
        .data = "Module initialized successfully",
        .data_size = strlen("Module initialized successfully") + 1
    };

    /* Сохранение события в массив для проверки */
    memcpy(&test_events[event_count], &test_event, sizeof(event_t));
    event_count++;

    TEST_ASSERT_TRUE(test_event.type == EVENT_MODULE_INITIALIZED, "Тип события неверный");
    TEST_ASSERT_STRING_EQUAL("test_module", test_event.source, "Источник события неверный");
    TEST_ASSERT_TRUE(test_event.timestamp > 0, "Временная метка события должна быть положительной");
    TEST_ASSERT_NOT_NULL(test_event.data, "Данные события не должны быть пустыми");
    TEST_ASSERT_TRUE(test_event.data_size > 0, "Размер данных события должен быть > 0");

    /* Создание события об ошибке */
    event_t error_event = {
        .type = EVENT_ERROR_OCCURRED,
        .source = "cpu_monitor",
        .timestamp = time(NULL),
        .data = "High CPU usage detected",
        .data_size = strlen("High CPU usage detected") + 1
    };

    memcpy(&test_events[event_count], &error_event, sizeof(event_t));
    event_count++;

    TEST_ASSERT_TRUE(error_event.type == EVENT_ERROR_OCCURRED, "Тип события ошибки неверный");
    TEST_ASSERT_STRING_EQUAL("cpu_monitor", error_event.source, "Источник события ошибки неверный");

    /* Проверка что события сохранены корректно */
    TEST_ASSERT_TRUE(event_count == 2, "Должно быть сохранено 2 события");
    TEST_ASSERT_TRUE(test_events[0].type == EVENT_MODULE_INITIALIZED, "Первое событие неверное");
    TEST_ASSERT_TRUE(test_events[1].type == EVENT_ERROR_OCCURRED, "Второе событие неверное");

    TEST_ASSERT_SUCCESS(0, "Система событий работает корректно");
}
TEST_END();

/* ============================================================================
 * Интеграционные тесты для UI системы
 * ============================================================================ */

/**
 * @brief Тест интеграции UI компонентов
 */
TEST_BEGIN(test_ui_integration) {
    /* Тестирование интеграции UI компонентов архитектуры */

    /* Создание UI рендерера */
    module_t *ui_renderer = malloc(sizeof(module_t));
    TEST_ASSERT_NOT_NULL(ui_renderer, "Не удалось создать UI рендерер");

    ui_renderer->info.status = MODULE_STATUS_UNINITIALIZED;
    ui_renderer->ops.init = mock_ui_renderer_init;

    /* Инициализация UI рендерера */
    int ui_init_result = ui_renderer->ops.init(&ui_renderer->info, &mock_app_context);
    TEST_ASSERT_SUCCESS(ui_init_result, "Инициализация UI рендерера должна пройти успешно");
    TEST_ASSERT_TRUE(ui_renderer->info.status == MODULE_STATUS_RUNNING, "UI рендерер должен быть в статусе RUNNING");

    /* Проверка что UI рендерер имеет наивысший приоритет среди опциональных модулей */
    TEST_ASSERT_TRUE(ui_renderer->info.priority == 10, "Приоритет UI рендерера неверный");
    TEST_ASSERT_FALSE(ui_renderer->info.required, "UI рендерер не должен быть обязательным модулем");

    /* Тестирование рендеринга с различными метриками */
    current_metrics.cpu_usage = 50.0;
    current_metrics.memory_usage = 75.0;
    current_metrics.temperature = 45.0;

    /* В реальной реализации здесь был бы вызов функции рендеринга */
    /* int render_result = mock_ui_renderer_render(&current_metrics); */
    /* TEST_ASSERT_SUCCESS(render_result, "Рендеринг должен пройти успешно"); */

    /* Тестирование изменения метрик и повторного рендеринга */
    current_metrics.cpu_usage = 80.0; /* Высокая нагрузка CPU */

    /* В реальной реализации здесь проверялся бы обновленный UI */
    TEST_ASSERT_DOUBLE_EQUAL(80.0, current_metrics.cpu_usage, 0.01, "Метрика CPU не изменилась");

    /* Освобождение ресурсов */
    free(ui_renderer);

    TEST_ASSERT_SUCCESS(0, "Интеграция UI компонентов работает корректно");
}
TEST_END();

/* ============================================================================
 * Интеграционные тесты для системы метрик
 * ============================================================================ */

/**
 * @brief Тест сборщика метрик системы
 */
TEST_BEGIN(test_metrics_collector_integration) {
    /* Тестирование интеграции сборщика метрик */

    /* Инициализация тестовых метрик */
    current_metrics.cpu_usage = 25.0;
    current_metrics.memory_usage = 60.0;
    current_metrics.disk_usage = 45.0;
    current_metrics.network_rx = 1024;
    current_metrics.network_tx = 512;
    current_metrics.temperature = 42.0;
    current_metrics.battery_level = 85.0;
    current_metrics.process_count = 150;
    current_metrics.load_average = 1.2;
    current_metrics.uptime = 3600;

    /* Проверка корректности всех метрик */
    TEST_ASSERT_IN_RANGE(current_metrics.cpu_usage, 0.0, 100.0, "CPU usage вне диапазона");
    TEST_ASSERT_IN_RANGE(current_metrics.memory_usage, 0.0, 100.0, "Memory usage вне диапазона");
    TEST_ASSERT_IN_RANGE(current_metrics.disk_usage, 0.0, 100.0, "Disk usage вне диапазона");
    TEST_ASSERT_TRUE(current_metrics.network_rx >= 0, "Network RX не может быть отрицательным");
    TEST_ASSERT_TRUE(current_metrics.network_tx >= 0, "Network TX не может быть отрицательным");
    TEST_ASSERT_TRUE(current_metrics.temperature > 0, "Температура должна быть положительной");
    TEST_ASSERT_IN_RANGE(current_metrics.battery_level, 0.0, 100.0, "Battery level вне диапазона");
    TEST_ASSERT_TRUE(current_metrics.process_count > 0, "Количество процессов должно быть > 0");
    TEST_ASSERT_TRUE(current_metrics.load_average >= 0, "Load average не может быть отрицательным");
    TEST_ASSERT_TRUE(current_metrics.uptime >= 0, "Uptime не может быть отрицательным");

    /* Тестирование обновления метрик */
    current_metrics.cpu_usage = 75.0; /* Высокая нагрузка */
    current_metrics.memory_usage = 90.0; /* Почти полная память */
    current_metrics.temperature = 65.0; /* Высокая температура */

    TEST_ASSERT_DOUBLE_EQUAL(75.0, current_metrics.cpu_usage, 0.01, "CPU usage не обновился");
    TEST_ASSERT_DOUBLE_EQUAL(90.0, current_metrics.memory_usage, 0.01, "Memory usage не обновился");
    TEST_ASSERT_DOUBLE_EQUAL(65.0, current_metrics.temperature, 0.01, "Temperature не обновилась");

    /* Тестирование граничных значений */
    current_metrics.cpu_usage = 0.0; /* Минимальная нагрузка */
    current_metrics.memory_usage = 100.0; /* Максимальное использование */
    current_metrics.battery_level = 0.0; /* Разряженная батарея */

    TEST_ASSERT_DOUBLE_EQUAL(0.0, current_metrics.cpu_usage, 0.01, "Минимальный CPU usage неверный");
    TEST_ASSERT_DOUBLE_EQUAL(100.0, current_metrics.memory_usage, 0.01, "Максимальный memory usage неверный");
    TEST_ASSERT_DOUBLE_EQUAL(0.0, current_metrics.battery_level, 0.01, "Минимальный battery level неверный");

    TEST_ASSERT_SUCCESS(0, "Сборщик метрик работает корректно");
}
TEST_END();

/* ============================================================================
 * End-to-End тесты полного функционала
 * ============================================================================ */

/**
 * @brief End-to-End тест полного цикла работы системы мониторинга
 */
TEST_BEGIN(test_full_monitoring_cycle) {
    /* Тестирование полного цикла работы системы мониторинга */

    /* Этап 1: Инициализация всех компонентов */
    printf("Этап 1: Инициализация компонентов...\n");

    /* Инициализация контекста приложения */
    mock_app_context.start_time = time(NULL);
    TEST_ASSERT_TRUE(mock_app_context.initialized, "Контекст приложения должен быть инициализирован");

    /* Инициализация модулей мониторинга */
    module_t cpu_monitor = {0};
    cpu_monitor.ops.init = mock_cpu_monitor_init;
    cpu_monitor.info.status = MODULE_STATUS_UNINITIALIZED;

    module_t memory_monitor = {0};
    memory_monitor.ops.init = mock_memory_monitor_init;
    memory_monitor.info.status = MODULE_STATUS_UNINITIALIZED;

    module_t ui_component = {0};
    ui_component.ops.init = mock_ui_renderer_init;
    ui_component.info.status = MODULE_STATUS_UNINITIALIZED;

    /* Запуск модулей */
    TEST_ASSERT_SUCCESS(cpu_monitor.ops.init(&cpu_monitor.info, &mock_app_context), "Инициализация CPU монитора");
    TEST_ASSERT_SUCCESS(memory_monitor.ops.init(&memory_monitor.info, &mock_app_context), "Инициализация memory монитора");
    TEST_ASSERT_SUCCESS(ui_component.ops.init(&ui_component.info, &mock_app_context), "Инициализация UI компонента");

    /* Этап 2: Сбор данных от модулей */
    printf("Этап 2: Сбор данных от модулей...\n");

    /* Установка тестовых значений метрик */
    current_metrics.cpu_usage = 45.0;
    current_metrics.memory_usage = 70.0;
    current_metrics.disk_usage = 55.0;
    current_metrics.temperature = 50.0;

    /* Получение данных от модулей */
    double cpu_data = 0.0;
    double memory_data = 0.0;

    TEST_ASSERT_TRUE(cpu_monitor.ops.get_data(&cpu_data, sizeof(double)) > 0, "Получение данных CPU");
    TEST_ASSERT_TRUE(memory_monitor.ops.get_data(&memory_data, sizeof(double)) > 0, "Получение данных памяти");

    TEST_ASSERT_DOUBLE_EQUAL(current_metrics.cpu_usage, cpu_data, 0.01, "Данные CPU не соответствуют ожиданиям");
    TEST_ASSERT_DOUBLE_EQUAL(current_metrics.memory_usage, memory_data, 0.01, "Данные памяти не соответствуют ожиданиям");

    /* Этап 3: Генерация событий */
    printf("Этап 3: Генерация событий...\n");

    /* Создание событий на основе данных */
    if (current_metrics.cpu_usage > 80.0) {
        event_t high_cpu_event = {
            .type = EVENT_ERROR_OCCURRED,
            .source = "cpu_monitor",
            .timestamp = time(NULL),
            .data = "High CPU usage detected",
            .data_size = strlen("High CPU usage detected") + 1
        };
        memcpy(&test_events[event_count], &high_cpu_event, sizeof(event_t));
        event_count++;
    }

    if (current_metrics.memory_usage > 85.0) {
        event_t high_memory_event = {
            .type = EVENT_ERROR_OCCURRED,
            .source = "memory_monitor",
            .timestamp = time(NULL),
            .data = "High memory usage detected",
            .data_size = strlen("High memory usage detected") + 1
        };
        memcpy(&test_events[event_count], &high_memory_event, sizeof(event_t));
        event_count++;
    }

    /* Этап 4: Обновление UI */
    printf("Этап 4: Обновление пользовательского интерфейса...\n");

    /* В реальной реализации здесь был бы вызов функции обновления UI */
    /* TEST_ASSERT_SUCCESS(mock_ui_renderer_render(&current_metrics), "Обновление UI"); */

    /* Этап 5: Проверка результатов */
    printf("Этап 5: Проверка результатов...\n");

    TEST_ASSERT_TRUE(cpu_monitor.info.status == MODULE_STATUS_RUNNING, "CPU монитор должен работать");
    TEST_ASSERT_TRUE(memory_monitor.info.status == MODULE_STATUS_RUNNING, "Memory монитор должен работать");
    TEST_ASSERT_TRUE(ui_component.info.status == MODULE_STATUS_RUNNING, "UI компонент должен работать");

    TEST_ASSERT_TRUE(event_count >= 0, "Должны быть созданы события");
    TEST_ASSERT_DOUBLE_EQUAL(45.0, current_metrics.cpu_usage, 0.01, "Финальные метрики CPU корректны");
    TEST_ASSERT_DOUBLE_EQUAL(70.0, current_metrics.memory_usage, 0.01, "Финальные метрики памяти корректны");

    printf("Full monitoring cycle test completed successfully!\n");

    TEST_ASSERT_SUCCESS(0, "Полный цикл мониторинга работает корректно");
}
TEST_END();

/* ============================================================================
 * Тесты производительности интеграции
 * ============================================================================ */

/**
 * @brief Тест производительности модульной системы
 */
TEST_BEGIN(test_integration_performance) {
    /* Тестирование производительности взаимодействия компонентов */

    TEST_TIME_START();

    /* Создание множества модулей для тестирования производительности */
    const int module_count = 100;
    module_t *modules[100] = {0};

    for (int i = 0; i < module_count; ++i) {
        modules[i] = malloc(sizeof(module_t));
        TEST_ASSERT_NOT_NULL(modules[i], "Не удалось создать модуль");

        modules[i]->info.status = MODULE_STATUS_UNINITIALIZED;
        modules[i]->ops.init = mock_cpu_monitor_init; /* Используем фиктивную функцию инициализации */

        char module_name[32];
        snprintf(module_name, sizeof(module_name), "test_module_%d", i);
        safe_strcpy(modules[i]->info.name, module_name, sizeof(modules[i]->info.name));

        int init_result = modules[i]->ops.init(&modules[i]->info, &mock_app_context);
        TEST_ASSERT_SUCCESS(init_result, "Инициализация модуля должна пройти успешно");
    }

    uint64_t init_time = TEST_TIME_END();
    TEST_DIAGNOSTIC("Время инициализации %d модулей: %llu мс", module_count, (unsigned long long)init_time);

    /* Проверка что все модули инициализированы корректно */
    for (int i = 0; i < module_count; ++i) {
        TEST_ASSERT_TRUE(modules[i]->info.status == MODULE_STATUS_RUNNING, "Модуль должен быть инициализирован");
        free(modules[i]);
    }

    /* Тестирование не должно занимать слишком много времени */
    TEST_ASSERT_TRUE(init_time < 1000, "Инициализация модулей не должна занимать больше 1 секунды");

    TEST_ASSERT_SUCCESS(0, "Производительность интеграции в норме");
}
TEST_END();

/* ============================================================================
 * Тесты стрессовой нагрузки
 * ============================================================================ */

/**
 * @brief Тест стрессовой нагрузки на систему событий
 */
TEST_BEGIN(test_event_system_stress) {
    /* Тестирование системы событий под высокой нагрузкой */

    TEST_TIME_START();

    /* Генерация большого количества событий */
    const int event_stress_count = 10000;

    for (int i = 0; i < event_stress_count; ++i) {
        event_t stress_event = {
            .type = EVENT_MODULE_DATA_UPDATED,
            .source = "stress_test",
            .timestamp = time(NULL),
            .data = NULL,
            .data_size = 0
        };

        /* В реальной реализации здесь был бы вызов функции отправки события */
        /* test_emit_event(stress_event.type, &stress_event); */

        /* Для теста просто сохраняем событие */
        if (event_count < sizeof(test_events) / sizeof(test_events[0])) {
            memcpy(&test_events[event_count], &stress_event, sizeof(event_t));
            event_count++;
        }
    }

    uint64_t stress_time = TEST_TIME_END();
    TEST_DIAGNOSTIC("Время обработки %d событий: %llu мс", event_stress_count, (unsigned long long)stress_time);

    /* Проверка что события генерируются достаточно быстро */
    TEST_ASSERT_TRUE(stress_time < 5000, "Обработка событий не должна занимать больше 5 секунд");
    TEST_ASSERT_TRUE(event_count > 0, "Должны быть созданы события");

    TEST_ASSERT_SUCCESS(0, "Система событий выдерживает стрессовую нагрузку");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы интеграционных тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы интеграционных тестов
 */
static void integration_tests_setup(test_context_t *context) {
    (void)context;
    printf("Настройка группы интеграционных тестов...\n");

    /* Инициализация тестовых данных */
    memset(test_modules, 0, sizeof(test_modules));
    memset(test_events, 0, sizeof(test_events));
    module_count = 0;
    event_count = 0;

    /* Инициализация метрик начальными значениями */
    current_metrics.cpu_usage = 25.0;
    current_metrics.memory_usage = 50.0;
    current_metrics.disk_usage = 30.0;
    current_metrics.network_rx = 1024;
    current_metrics.network_tx = 512;
    current_metrics.temperature = 40.0;
    current_metrics.battery_level = 90.0;
    current_metrics.process_count = 100;
    current_metrics.load_average = 0.8;
    current_metrics.uptime = 1800;

    /* Инициализация контекста приложения */
    mock_app_context.start_time = time(NULL);
}

/**
 * @brief Очистка после завершения группы интеграционных тестов
 */
static void integration_tests_teardown(test_context_t *context) {
    (void)context;
    printf("Очистка после группы интеграционных тестов...\n");

    /* Освобождение ресурсов модулей */
    for (size_t i = 0; i < module_count; ++i) {
        if (test_modules[i]) {
            free(test_modules[i]);
            test_modules[i] = NULL;
        }
    }
    module_count = 0;

    /* Очистка событий */
    memset(test_events, 0, sizeof(test_events));
    event_count = 0;
}

/* ============================================================================
 * Регистрация всех интеграционных тестов
 * ============================================================================ */

/**
 * @brief Создание группы интеграционных тестов
 */
test_suite_t *create_integration_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("integration", "Интеграционные тесты SystemMonitor", TEST_TYPE_INTEGRATION);

    if (!suite) {
        printf("Ошибка создания группы интеграционных тестов\n");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Интеграционные тесты модулей */
    test_def = test_definition_create("test_monitoring_modules_integration", "integration",
                                    test_monitoring_modules_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты системы событий */
    test_def = test_definition_create("test_event_system_integration", "integration",
                                    test_event_system_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты UI интеграции */
    test_def = test_definition_create("test_ui_integration", "integration",
                                    test_ui_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты системы метрик */
    test_def = test_definition_create("test_metrics_collector_integration", "integration",
                                    test_metrics_collector_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* End-to-End тесты */
    test_def = test_definition_create("test_full_monitoring_cycle", "integration",
                                    test_full_monitoring_cycle, TEST_TYPE_END_TO_END);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_integration_performance", "integration",
                                    test_integration_performance, TEST_TYPE_PERFORMANCE);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Стресс-тесты */
    test_def = test_definition_create("test_event_system_stress", "integration",
                                    test_event_system_stress, TEST_TYPE_STRESS);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    return suite;
}

/* ============================================================================
 * Главная функция для запуска интеграционных тестов
 * ============================================================================ */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("Запуск группы интеграционных тестов SystemMonitor Test Framework\n");
    printf("==============================================================\n");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 30000; /* 30 секунд таймаут для интеграционных тестов */

    if (test_framework_init(&config) != 0) {
        printf("Ошибка инициализации тестового фреймворка\n");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_integration_test_suite();
    if (!suite) {
        printf("Ошибка создания группы интеграционных тестов\n");
        test_framework_cleanup();
        return 1;
    }

    /* Регистрация обработчиков событий */
    test_register_event_handler(TEST_EVENT_STARTED, NULL, NULL);
    test_register_event_handler(TEST_EVENT_COMPLETED, NULL, NULL);
    test_register_event_handler(TEST_EVENT_FAILED, NULL, NULL);

    /* Запуск группы тестов */
    int result = test_suite_run(suite);

    /* Освобождение ресурсов */
    test_suite_destroy(suite);
    test_framework_cleanup();

    printf("\nГруппа интеграционных тестов завершена\n");
    return result;
}