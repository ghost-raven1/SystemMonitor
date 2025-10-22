/**
 * @file test_example.c
 * @brief Пример unit тестов для базовых компонентов SystemMonitor
 *
 * Этот файл демонстрирует использование тестового фреймворка для
 * тестирования базовых функций и структур данных архитектуры SystemMonitor.
 */

#include "../../framework/include/test_framework.h"
#include "../../framework/include/test_macros.h"
#include "../../framework/include/test_asserts.h"
#include "../../framework/include/system_monitor_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * Тестовые данные и вспомогательные функции
 * ============================================================================ */

/**
 * @brief Структура для тестирования базовых типов данных
 */
typedef struct {
    int id;
    char name[64];
    double value;
    bool enabled;
} test_data_t;

/**
 * @brief Функция для создания тестовых данных
 */
test_data_t *create_test_data(int id, const char *name, double value, bool enabled) {
    test_data_t *data = malloc(sizeof(test_data_t));
    if (!data) {
        return NULL;
    }

    data->id = id;
    safe_strcpy(data->name, name, sizeof(data->name));
    data->value = value;
    data->enabled = enabled;

    return data;
}

/**
 * @brief Функция для освобождения тестовых данных
 */
void destroy_test_data(test_data_t *data) {
    if (data) {
        free(data);
    }
}

/* ============================================================================
 * Unit тесты для базовых операций с памятью
 * ============================================================================ */

TEST_BEGIN(test_memory_allocation) {
    /* Тестирование выделения памяти */
    test_data_t *data = malloc(sizeof(test_data_t));
    TEST_ASSERT_NOT_NULL(data, "Не удалось выделить память для test_data_t");

    /* Инициализация данных */
    data->id = 42;
    safe_strcpy(data->name, "test_item", sizeof(data->name));
    data->value = 3.14159;
    data->enabled = true;

    /* Проверка инициализации */
    TEST_ASSERT_EQUAL(42, data->id, "Неверный ID после инициализации");
    TEST_ASSERT_STRING_EQUAL("test_item", data->name, "Неверное имя после инициализации");
    TEST_ASSERT_DOUBLE_EQUAL(3.14159, data->value, 0.0001, "Неверное значение после инициализации");
    TEST_ASSERT_TRUE(data->enabled, "Флаг enabled должен быть true");

    /* Освобождение ресурсов */
    free(data);
    TEST_ASSERT_SUCCESS(0, "Тест завершен успешно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для строковых операций
 * ============================================================================ */

TEST_BEGIN(test_string_operations) {
    char buffer[256];

    /* Тестирование безопасного копирования строк */
    safe_strcpy(buffer, "Hello, World!", sizeof(buffer));
    TEST_ASSERT_STRING_EQUAL("Hello, World!", buffer, "Ошибка копирования строки");

    /* Тестирование копирования слишком длинной строки */
    safe_strcpy(buffer, "Это очень длинная строка, которая должна быть усечена при копировании в буфер ограниченного размера", sizeof(buffer));
    TEST_ASSERT_TRUE(strlen(buffer) < 256, "Строка не была усечена");

    /* Тестирование операций сравнения */
    const char *str1 = "test_string";
    const char *str2 = "test_string";
    const char *str3 = "different_string";

    TEST_ASSERT_TRUE(strcmp(str1, str2) == 0, "Строки str1 и str2 должны быть равны");
    TEST_ASSERT_TRUE(strcmp(str1, str3) != 0, "Строки str1 и str3 должны быть разными");

    TEST_ASSERT_SUCCESS(0, "Строковые операции работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для структур данных архитектуры SystemMonitor
 * ============================================================================ */

TEST_BEGIN(test_metrics_structure) {
    /* Тестирование структуры метрик в соответствии с API спецификацией */
    metric_t cpu_metric = {
        .name = "cpu_usage",
        .unit = "%",
        .value = 45.67,
        .min_value = 0.0,
        .max_value = 100.0,
        .average = 42.1,
        .timestamp = 1234567890,
        .data_points = 100
    };

    TEST_ASSERT_STRING_EQUAL("cpu_usage", cpu_metric.name, "Неверное имя метрики");
    TEST_ASSERT_STRING_EQUAL("%", cpu_metric.unit, "Неверная единица измерения");
    TEST_ASSERT_DOUBLE_EQUAL(45.67, cpu_metric.value, 0.01, "Неверное значение метрики");
    TEST_ASSERT_TRUE(cpu_metric.min_value >= 0.0, "Минимальное значение должно быть >= 0");
    TEST_ASSERT_TRUE(cpu_metric.max_value <= 100.0, "Максимальное значение должно быть <= 100");
    TEST_ASSERT_TRUE(cpu_metric.data_points > 0, "Количество точек данных должно быть > 0");

    /* Тестирование граничных значений */
    cpu_metric.value = 0.0;
    TEST_ASSERT_DOUBLE_EQUAL(0.0, cpu_metric.value, 0.01, "Минимальное значение CPU");

    cpu_metric.value = 100.0;
    TEST_ASSERT_DOUBLE_EQUAL(100.0, cpu_metric.value, 0.01, "Максимальное значение CPU");

    TEST_ASSERT_SUCCESS(0, "Структура метрик работает корректно");
}
TEST_END();

TEST_BEGIN(test_system_metrics_structure) {
    /* Тестирование полной структуры системных метрик */
    system_metrics_t metrics = {0};

    /* Инициализация метрик */
    metrics.cpu_usage = 25.5;
    metrics.memory_usage = 60.7;
    metrics.disk_usage = 45.2;
    metrics.network_rx = 1024;
    metrics.network_tx = 512;
    metrics.temperature = 42.0;
    metrics.battery_level = 85.0;
    metrics.process_count = 150;
    metrics.load_average = 1.2;
    metrics.uptime = 3600;

    /* Проверка корректности значений */
    TEST_ASSERT_IN_RANGE(metrics.cpu_usage, 0.0, 100.0, "CPU usage вне диапазона");
    TEST_ASSERT_IN_RANGE(metrics.memory_usage, 0.0, 100.0, "Memory usage вне диапазона");
    TEST_ASSERT_IN_RANGE(metrics.disk_usage, 0.0, 100.0, "Disk usage вне диапазона");
    TEST_ASSERT_TRUE(metrics.network_rx >= 0, "Network RX не может быть отрицательным");
    TEST_ASSERT_TRUE(metrics.network_tx >= 0, "Network TX не может быть отрицательным");
    TEST_ASSERT_TRUE(metrics.temperature > 0, "Температура должна быть положительной");
    TEST_ASSERT_IN_RANGE(metrics.battery_level, 0.0, 100.0, "Battery level вне диапазона");
    TEST_ASSERT_TRUE(metrics.process_count > 0, "Количество процессов должно быть > 0");
    TEST_ASSERT_TRUE(metrics.load_average >= 0, "Load average не может быть отрицательным");
    TEST_ASSERT_TRUE(metrics.uptime >= 0, "Uptime не может быть отрицательным");

    TEST_ASSERT_SUCCESS(0, "Системные метрики корректны");
}
TEST_END();

/* ============================================================================
 * Unit тесты для конфигурационной системы
 * ============================================================================ */

TEST_BEGIN(test_config_structure) {
    /* Тестирование структуры конфигурации */
    config_entry_t config_entry = {
        .key = "max_cpu_usage",
        .type = CONFIG_TYPE_DOUBLE,
        .value.double_value = 80.0,
        .description = "Максимально допустимое использование CPU",
        .required = true
    };

    TEST_ASSERT_STRING_EQUAL("max_cpu_usage", config_entry.key, "Неверный ключ конфигурации");
    TEST_ASSERT_TRUE(config_entry.type == CONFIG_TYPE_DOUBLE, "Неверный тип значения");
    TEST_ASSERT_DOUBLE_EQUAL(80.0, config_entry.value.double_value, 0.01, "Неверное значение конфигурации");
    TEST_ASSERT_TRUE(config_entry.required, "Параметр должен быть обязательным");
    TEST_ASSERT_NOT_NULL(config_entry.description, "Описание не может быть пустым");

    /* Тестирование различных типов конфигурации */
    config_entry.type = CONFIG_TYPE_INT;
    config_entry.value.int_value = 42;
    TEST_ASSERT_TRUE(config_entry.value.int_value == 42, "Целочисленное значение неверное");

    config_entry.type = CONFIG_TYPE_BOOL;
    config_entry.value.bool_value = true;
    TEST_ASSERT_TRUE(config_entry.value.bool_value, "Логическое значение неверное");

    config_entry.type = CONFIG_TYPE_STRING;
    safe_strcpy(config_entry.value.string_value, "test_value", sizeof(config_entry.value.string_value));
    TEST_ASSERT_STRING_EQUAL("test_value", config_entry.value.string_value, "Строковое значение неверное");

    TEST_ASSERT_SUCCESS(0, "Структура конфигурации работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для системы событий
 * ============================================================================ */

TEST_BEGIN(test_event_structure) {
    /* Тестирование структуры событий архитектуры SystemMonitor */
    event_t test_event = {
        .type = EVENT_MODULE_INITIALIZED,
        .source = "test_module",
        .timestamp = 1234567890,
        .data = NULL,
        .data_size = 0
    };

    TEST_ASSERT_TRUE(test_event.type == EVENT_MODULE_INITIALIZED, "Неверный тип события");
    TEST_ASSERT_STRING_EQUAL("test_module", test_event.source, "Неверный источник события");
    TEST_ASSERT_TRUE(test_event.timestamp > 0, "Временная метка должна быть положительной");
    TEST_ASSERT_TRUE(test_event.data_size >= 0, "Размер данных не может быть отрицательным");

    /* Тестирование различных типов событий */
    test_event.type = EVENT_ERROR_OCCURRED;
    TEST_ASSERT_TRUE(test_event.type == EVENT_ERROR_OCCURRED, "Тип события не изменился");

    /* Тестирование с данными */
    char event_data[] = "test error message";
    test_event.data = event_data;
    test_event.data_size = sizeof(event_data);
    TEST_ASSERT_NOT_NULL(test_event.data, "Данные события должны быть установлены");
    TEST_ASSERT_TRUE(test_event.data_size > 0, "Размер данных должен быть > 0");

    TEST_ASSERT_SUCCESS(0, "Структура событий работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для модульной системы
 * ============================================================================ */

TEST_BEGIN(test_module_info_structure) {
    /* Тестирование структуры информации о модуле */
    module_info_t module_info = {
        .name = "test_module",
        .version = "1.0.0",
        .description = "Тестовый модуль для демонстрации",
        .status = MODULE_STATUS_RUNNING,
        .priority = 5,
        .required = true,
        .private_data = NULL
    };

    TEST_ASSERT_STRING_EQUAL("test_module", module_info.name, "Неверное имя модуля");
    TEST_ASSERT_STRING_EQUAL("1.0.0", module_info.version, "Неверная версия модуля");
    TEST_ASSERT_NOT_NULL(module_info.description, "Описание модуля не может быть пустым");
    TEST_ASSERT_TRUE(module_info.status == MODULE_STATUS_RUNNING, "Неверный статус модуля");
    TEST_ASSERT_TRUE(module_info.priority >= 0, "Приоритет не может быть отрицательным");
    TEST_ASSERT_TRUE(module_info.required, "Модуль должен быть обязательным");

    /* Тестирование различных статусов */
    module_info.status = MODULE_STATUS_INITIALIZING;
    TEST_ASSERT_TRUE(module_info.status == MODULE_STATUS_INITIALIZING, "Статус не изменился");

    module_info.status = MODULE_STATUS_ERROR;
    TEST_ASSERT_TRUE(module_info.status == MODULE_STATUS_ERROR, "Статус не изменился");

    TEST_ASSERT_SUCCESS(0, "Структура информации о модуле работает корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для работы с файлами
 * ============================================================================ */

TEST_BEGIN(test_file_operations) {
    /* Тестирование создания временного файла */
    char *temp_file = test_create_temp_file("test_prefix", ".txt");
    TEST_ASSERT_NOT_NULL(temp_file, "Не удалось создать временный файл");

    /* Проверка существования файла */
    TEST_ASSERT_FILE_EXISTS(temp_file, "Временный файл должен существовать");

    /* Тестирование записи в файл */
    FILE *fp = fopen(temp_file, "w");
    TEST_ASSERT_NOT_NULL(fp, "Не удалось открыть файл для записи");

    const char *test_content = "Это тестовый контент для проверки записи в файл.";
    size_t written = fwrite(test_content, 1, strlen(test_content), fp);
    TEST_ASSERT_TRUE(written == strlen(test_content), "Не удалось записать контент в файл");

    fclose(fp);

    /* Проверка размера файла */
    TEST_ASSERT_FILE_SIZE(temp_file, strlen(test_content), "Размер файла неверный");

    /* Тестирование чтения из файла */
    fp = fopen(temp_file, "r");
    TEST_ASSERT_NOT_NULL(fp, "Не удалось открыть файл для чтения");

    char read_buffer[256] = {0};
    size_t read_size = fread(read_buffer, 1, sizeof(read_buffer) - 1, fp);
    TEST_ASSERT_TRUE(read_size == strlen(test_content), "Не удалось прочитать контент из файла");
    TEST_ASSERT_STRING_EQUAL(test_content, read_buffer, "Прочитанный контент не совпадает");

    fclose(fp);

    /* Удаление временного файла */
    test_remove_temp_file(temp_file);
    TEST_ASSERT_FILE_NOT_EXISTS(temp_file, "Временный файл должен быть удален");

    TEST_ASSERT_SUCCESS(0, "Файловые операции работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для математических операций
 * ============================================================================ */

TEST_BEGIN(test_mathematical_operations) {
    /* Тестирование базовых математических операций */

    /* Целочисленная арифметика */
    int a = 10, b = 3;
    TEST_ASSERT_EQUAL(13, a + b, "Сложение работает неверно");
    TEST_ASSERT_EQUAL(7, a - b, "Вычитание работает неверно");
    TEST_ASSERT_EQUAL(30, a * b, "Умножение работает неверно");
    TEST_ASSERT_EQUAL(3, a / b, "Деление работает неверно");
    TEST_ASSERT_EQUAL(1, a % b, "Деление по модулю работает неверно");

    /* Арифметика с плавающей точкой */
    double x = 10.5, y = 2.5;
    TEST_ASSERT_DOUBLE_EQUAL(13.0, x + y, 0.001, "Сложение с плавающей точкой работает неверно");
    TEST_ASSERT_DOUBLE_EQUAL(8.0, x - y, 0.001, "Вычитание с плавающей точкой работает неверно");
    TEST_ASSERT_DOUBLE_EQUAL(26.25, x * y, 0.001, "Умножение с плавающей точкой работает неверно");
    TEST_ASSERT_DOUBLE_EQUAL(4.2, x / y, 0.001, "Деление с плавающей точкой работает неверно");

    /* Граничные случаи */
    TEST_ASSERT_DOUBLE_EQUAL(0.0, 0.0 + 0.0, 0.001, "Сложение нулей");
    TEST_ASSERT_TRUE(isinf(1.0 / 0.0), "Деление на ноль должно давать бесконечность");
    TEST_ASSERT_TRUE(isnan(0.0 / 0.0), "Деление нуля на ноль должно давать NaN");

    TEST_ASSERT_SUCCESS(0, "Математические операции работают корректно");
}
TEST_END();

/* ============================================================================
 * Unit тесты для производительности и памяти
 * ============================================================================ */

TEST_BEGIN(test_performance_monitoring) {
    /* Тестирование мониторинга производительности */
    TEST_TIME_START();

    /* Имитация работы */
    volatile int counter = 0;
    for (int i = 0; i < 100000; ++i) {
        counter += i;
    }

    uint64_t elapsed_time = TEST_TIME_END();
    TEST_DIAGNOSTIC("Время выполнения цикла: %llu мс", (unsigned long long)elapsed_time);

    /* Проверка что время измерено */
    TEST_ASSERT_TRUE(elapsed_time > 0, "Время выполнения должно быть больше 0");
    TEST_ASSERT_TRUE(elapsed_time < 1000, "Цикл не должен выполняться больше 1 секунды");

    /* Тестирование использования памяти */
    uint64_t memory_before = test_get_memory_usage();
    TEST_DIAGNOSTIC("Использование памяти до: %llu байт", (unsigned long long)memory_before);

    /* Выделение памяти для тестирования */
    char *test_buffer = malloc(1024 * 1024); /* 1 MB */
    TEST_ASSERT_NOT_NULL(test_buffer, "Не удалось выделить тестовый буфер");

    uint64_t memory_after = test_get_memory_usage();
    TEST_DIAGNOSTIC("Использование памяти после: %llu байт", (unsigned long long)memory_after);

    /* Освобождение ресурсов */
    free(test_buffer);

    TEST_ASSERT_SUCCESS(0, "Мониторинг производительности работает корректно");
}
TEST_END();

/* ============================================================================
 * Интеграционные тесты для взаимодействия компонентов
 * ============================================================================ */

/**
 * @brief Фиктивная функция модуля для тестирования интеграции
 */
static int mock_module_init(module_info_t *info, const app_context_t *ctx) {
    (void)ctx;
    if (info) {
        safe_strcpy(info->name, "mock_module", sizeof(info->name));
        safe_strcpy(info->version, "1.0.0", sizeof(info->version));
        info->status = MODULE_STATUS_RUNNING;
        return 0;
    }
    return -1;
}

/**
 * @brief Фиктивная функция получения данных модуля
 */
static int mock_module_get_data(void *buffer, size_t buffer_size) {
    if (buffer && buffer_size > 0) {
        snprintf((char *)buffer, buffer_size, "mock_data_%zu", buffer_size);
        return 0;
    }
    return -1;
}

/**
 * @brief Фиктивная функция обработки событий модуля
 */
static int mock_module_handle_event(const char *event_type, void *event_data) {
    (void)event_type;
    (void)event_data;
    return 0;
}

TEST_BEGIN(test_module_integration) {
    /* Тестирование интеграции модульной системы */

    /* Создание фиктивного модуля */
    module_t mock_module = {
        .info = {
            .name = "integration_test_module",
            .version = "2.0.0",
            .description = "Модуль для интеграционного тестирования",
            .status = MODULE_STATUS_UNINITIALIZED,
            .priority = 10,
            .required = false,
            .private_data = NULL
        },
        .ops = {
            .init = mock_module_init,
            .start = NULL,
            .stop = NULL,
            .pause = NULL,
            .resume = NULL,
            .cleanup = NULL,
            .get_data = mock_module_get_data,
            .handle_event = mock_module_handle_event,
            .get_last_error = NULL
        }
    };

    /* Тестирование инициализации модуля */
    TEST_ASSERT_TRUE(mock_module.info.status == MODULE_STATUS_UNINITIALIZED,
                     "Начальный статус модуля должен быть UNINITIALIZED");

    /* Имитация инициализации модуля */
    int init_result = mock_module_init(&mock_module.info, NULL);
    TEST_ASSERT_SUCCESS(init_result, "Инициализация модуля должна пройти успешно");
    TEST_ASSERT_TRUE(mock_module.info.status == MODULE_STATUS_RUNNING,
                     "После инициализации статус должен быть RUNNING");

    /* Тестирование получения данных от модуля */
    char data_buffer[256] = {0};
    int data_result = mock_module_get_data(data_buffer, sizeof(data_buffer));
    TEST_ASSERT_SUCCESS(data_result, "Получение данных модуля должно пройти успешно");
    TEST_ASSERT_NOT_NULL(data_buffer, "Буфер данных не должен быть пустым");
    TEST_ASSERT_TRUE(strlen(data_buffer) > 0, "Данные модуля не должны быть пустыми");

    /* Тестирование обработки событий */
    int event_result = mock_module_handle_event("test_event", data_buffer);
    TEST_ASSERT_SUCCESS(event_result, "Обработка событий модуля должна пройти успешно");

    TEST_ASSERT_SUCCESS(0, "Интеграция модулей работает корректно");
}
TEST_END();

/* ============================================================================
 * Тесты для системы плагинов
 * ============================================================================ */

TEST_BEGIN(test_plugin_system) {
    /* Тестирование системы плагинов архитектуры SystemMonitor */

    /* Создание фиктивного плагина */
    plugin_t mock_plugin = {
        .info = {
            .name = "test_plugin",
            .version = "1.0.0",
            .description = "Тестовый плагин для демонстрации",
            .type = PLUGIN_TYPE_MONITORING,
            .author = "Test Developer",
            .license = "MIT",
            .dependencies = NULL,
            .enabled = true,
            .load_order = 5
        },
        .interface = {
            .init = NULL,
            .start = NULL,
            .stop = NULL,
            .cleanup = NULL,
            .get_plugin_info = NULL,
            .handle_event = NULL,
            .get_last_error = NULL,
            .validate_config = NULL
        },
        .handle = NULL,
        .file_path = "/path/to/plugin.so",
        .load_time = 1234567890,
        .initialized = false
    };

    /* Проверка информации о плагине */
    TEST_ASSERT_STRING_EQUAL("test_plugin", mock_plugin.info.name, "Неверное имя плагина");
    TEST_ASSERT_STRING_EQUAL("1.0.0", mock_plugin.info.version, "Неверная версия плагина");
    TEST_ASSERT_TRUE(mock_plugin.info.type == PLUGIN_TYPE_MONITORING, "Неверный тип плагина");
    TEST_ASSERT_TRUE(mock_plugin.info.enabled, "Плагин должен быть включен");
    TEST_ASSERT_TRUE(mock_plugin.info.load_order >= 0, "Порядок загрузки должен быть >= 0");

    /* Проверка пути к файлу плагина */
    TEST_ASSERT_NOT_NULL(mock_plugin.file_path, "Путь к файлу плагина не может быть пустым");
    TEST_ASSERT_TRUE(strlen(mock_plugin.file_path) > 0, "Путь к файлу плагина не может быть пустым");

    /* Проверка времени загрузки */
    TEST_ASSERT_TRUE(mock_plugin.load_time > 0, "Время загрузки должно быть положительным");

    TEST_ASSERT_SUCCESS(0, "Система плагинов работает корректно");
}
TEST_END();

/* ============================================================================
 * Функции настройки и очистки для группы тестов
 * ============================================================================ */

/**
 * @brief Настройка перед запуском группы тестов
 */
static void core_tests_setup(test_context_t *context) {
    (void)context;
    printf("Настройка группы core тестов...\n");

    /* Инициализация генератора случайных чисел */
    srand((unsigned int)time(NULL));

    /* Создание тестовых директорий */
    mkdir("./test_temp", 0755);
}

/**
 * @brief Очистка после завершения группы тестов
 */
static void core_tests_teardown(test_context_t *context) {
    (void)context;
    printf("Очистка после группы core тестов...\n");

    /* Удаление тестовых директорий */
    rmdir("./test_temp");

    /* Освобождение ресурсов */
    /* В реальном сценарии здесь освобождались бы глобальные ресурсы */
}

/* ============================================================================
 * Регистрация всех тестов
 * ============================================================================ */

/**
 * @brief Создание группы core тестов
 */
test_suite_t *create_core_test_suite(void) {
    /* Создание группы тестов */
    test_suite_t *suite = test_suite_create("core", "Базовые компоненты SystemMonitor", TEST_TYPE_UNIT);

    if (!suite) {
        printf("Ошибка создания группы тестов core\n");
        return NULL;
    }

    /* Регистрация тестов */
    test_definition_t *test_def;

    /* Тесты памяти */
    test_def = test_definition_create("test_memory_allocation", "core", test_memory_allocation, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты строк */
    test_def = test_definition_create("test_string_operations", "core", test_string_operations, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты структур данных */
    test_def = test_definition_create("test_metrics_structure", "core", test_metrics_structure, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_system_metrics_structure", "core", test_system_metrics_structure, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_config_structure", "core", test_config_structure, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_event_structure", "core", test_event_structure, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_module_info_structure", "core", test_module_info_structure, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты файлов */
    test_def = test_definition_create("test_file_operations", "core", test_file_operations, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты математики */
    test_def = test_definition_create("test_mathematical_operations", "core", test_mathematical_operations, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Тесты производительности */
    test_def = test_definition_create("test_performance_monitoring", "core", test_performance_monitoring, TEST_TYPE_UNIT);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    /* Интеграционные тесты */
    test_def = test_definition_create("test_module_integration", "core", test_module_integration, TEST_TYPE_INTEGRATION);
    test_suite_add_test(suite, test_def);
    test_definition_destroy(test_def);

    test_def = test_definition_create("test_plugin_system", "core", test_plugin_system, TEST_TYPE_INTEGRATION);
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

    printf("Запуск группы core тестов SystemMonitor Test Framework\n");
    printf("======================================================\n");

    /* Инициализация фреймворка */
    test_config_t config = {0};
    config.verbose = true;
    config.colored_output = true;
    config.default_timeout_ms = 10000; /* 10 секунд таймаут */

    if (test_framework_init(&config) != 0) {
        printf("Ошибка инициализации тестового фреймворка\n");
        return 1;
    }

    /* Создание группы тестов */
    test_suite_t *suite = create_core_test_suite();
    if (!suite) {
        printf("Ошибка создания группы тестов\n");
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

    printf("\nГруппа core тестов завершена\n");
    return result;
}