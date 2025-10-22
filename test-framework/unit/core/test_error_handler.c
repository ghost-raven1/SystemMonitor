/**
 * @file test_error_handler.c
 * @brief Модульные тесты для системы обработки ошибок
 */

#include "test_framework.h"
#include "test_asserts.h"
#include "test_macros.h"
#include "error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Тестовые функции
static int test_error_creation(void) {
    printf("Testing error creation...\n");

    app_error_t error = create_error(
        ERR_BATTERY_INIT, SEVERITY_ERROR,
        "Test error message", "Test context", "test_module"
    );

    assert(error.code == ERR_BATTERY_INIT);
    assert(error.severity == SEVERITY_ERROR);
    assert(strcmp(error.message, "Test error message") == 0);
    assert(strcmp(error.context, "Test context") == 0);
    assert(strcmp(error.module, "test_module") == 0);
    assert(error.recoverable == true); // Ошибки ниже CRITICAL восстанавливаемы

    printf("✓ Error creation test passed\n");
    return 0;
}

static int test_module_management(void) {
    printf("Testing module management...\n");

    // Проверяем что модуль изначально включен
    assert(is_module_enabled("battery") == true);

    // Отключаем модуль
    disable_module("battery", "Test disable");
    assert(is_module_enabled("battery") == false);
    assert(is_module_failed("battery") == true);

    // Включаем модуль обратно
    enable_module("battery");
    assert(is_module_enabled("battery") == true);
    assert(is_module_failed("battery") == false);

    printf("✓ Module management test passed\n");
    return 0;
}

static int test_graceful_degradation(void) {
    printf("Testing graceful degradation...\n");

    // Инициализируем конфигурацию с включенной graceful degradation
    error_config_t config = {
        .enable_graceful_degradation = true,
        .enable_auto_recovery = false,
        .max_recovery_attempts = 3,
        .recovery_cooldown_sec = 1,
        .log_file_path = "",
        .custom_handler = NULL
    };

    error_handler_init(&config);

    // Регистрируем критическую ошибку для отключения модуля
    app_error_t error = create_error(
        ERR_BATTERY_INIT, SEVERITY_CRITICAL,
        "Critical battery failure", "Hardware fault", "battery"
    );

    register_error(&error);

    // Проверяем что модуль отключен
    assert(is_module_enabled("battery") == false);

    error_handler_cleanup();
    printf("✓ Graceful degradation test passed\n");
    return 0;
}

static int test_recovery_mechanism(void) {
    printf("Testing recovery mechanism...\n");

    // Инициализируем конфигурацию с включенным автопосстановлением
    error_config_t config = {
        .enable_graceful_degradation = true,
        .enable_auto_recovery = true,
        .max_recovery_attempts = 2,
        .recovery_cooldown_sec = 1,
        .log_file_path = "",
        .custom_handler = NULL
    };

    error_handler_init(&config);

    // Создаем модуль с ошибкой
    disable_module("test_module", "Test failure");

    // Проверяем что восстановление возможно
    assert(should_attempt_recovery("test_module") == true);

    // Пытаемся восстановить
    attempt_module_recovery("test_module");
    assert(is_module_enabled("test_module") == true);

    error_handler_cleanup();
    printf("✓ Recovery mechanism test passed\n");
    return 0;
}

static int test_macro_usage(void) {
    printf("Testing macro usage...\n");

    // Тестируем макросы логирования
    LOG_INFO("Test info message", "Macro test");
    LOG_WARNING(ERR_BATTERY_READ, "Test warning", "Macro test");
    LOG_ERROR(ERR_BATTERY_INIT, "Test error", "Macro test");
    LOG_CRITICAL(ERR_SYSTEM_MEMORY, "Test critical", "Macro test");
    LOG_DEBUG(100, "Test debug", "Macro test");

    printf("✓ Macro usage test passed\n");
    return 0;
}

// Кастомный обработчик ошибок для тестирования
static void test_custom_handler(const app_error_t *error) {
    printf("Custom handler called for: %s\n", error->message);
}

static int test_custom_handler(void) {
    printf("Testing custom handler...\n");

    error_config_t config = {
        .enable_graceful_degradation = false,
        .enable_auto_recovery = false,
        .max_recovery_attempts = 3,
        .recovery_cooldown_sec = 1,
        .log_file_path = "",
        .custom_handler = test_custom_handler
    };

    error_handler_init(&config);

    // Создаем ошибку, которая вызовет кастомный обработчик
    app_error_t error = create_error(
        ERR_BATTERY_INIT, SEVERITY_INFO,
        "Custom handler test", "Test context", "test_module"
    );

    register_error(&error);

    error_handler_cleanup();
    printf("✓ Custom handler test passed\n");
    return 0;
}

int main(void) {
    printf("Starting error handler test suite...\n\n");

    // Запускаем тесты
    int tests_passed = 0;
    int total_tests = 6;

    if (test_error_creation() == 0) tests_passed++;
    if (test_module_management() == 0) tests_passed++;
    if (test_graceful_degradation() == 0) tests_passed++;
    if (test_recovery_mechanism() == 0) tests_passed++;
    if (test_macro_usage() == 0) tests_passed++;
    if (test_custom_handler_test() == 0) tests_passed++;

    printf("\nTest Results: %d/%d tests passed\n", tests_passed, total_tests);

    if (tests_passed == total_tests) {
        printf("🎉 All tests PASSED!\n");
        return 0;
    } else {
        printf("❌ Some tests FAILED\n");
        return -1;
    }
}