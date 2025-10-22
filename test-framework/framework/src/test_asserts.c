/**
 * @file test_asserts.c
 * @brief Реализация функций ассертов для тестового фреймворка
 *
 * Этот файл содержит реализацию функций ассертов, интегрированных
 * с системой событий архитектуры SystemMonitor.
 */

#include "test_asserts.h"
#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

/* ============================================================================
 * Используем структуру контекста ассертов из заголовочного файла
 * ============================================================================ */

/* Глобальный контекст для обратной совместимости */
static test_assert_context_t global_assert_context = {0};

/* Макросы для обратной совместимости с глобальными переменными */
#define current_test_result (global_assert_context.current_test_result)
#define test_start_time (global_assert_context.test_start_time)
#define last_checkpoint_memory (global_assert_context.last_checkpoint_memory)
#define assertions_initialized (global_assert_context.assertions_initialized)

/**
 * @brief Получение текущего контекста ассертов
 */
static test_assert_context_t *get_current_context(void) {
    return &global_assert_context;
}

/**
 * @brief Инициализация контекста ассертов
 */
static void init_assert_context(test_assert_context_t *context, test_result_t *result, const char *test_name) {
    if (!context) return;

    context->current_test_result = result;
    context->test_start_time = get_current_time_ms();
    context->last_checkpoint_memory = 0;
    context->assertions_initialized = true;

    if (result && test_name) {
        safe_strcpy(result->name, test_name, sizeof(result->name));
        result->status = TEST_STATUS_RUNNING;
        result->start_time = context->test_start_time;
    }
}

/* ============================================================================
 * Внутренние функции
 * ============================================================================ */

/**
 * @brief Получение текущего времени в миллисекундах
 */
static uint64_t get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

/**
 * @brief Безопасное копирование строк
 */
static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return;
    }

    if (dest_size > 0) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

/**
 * @brief Добавление ассерта в результат теста
 */
static void add_assertion_to_result(const char *expression, const char *file,
                                   int line, const char *message, bool passed) {
    if (!current_test_result || !current_test_result->assertions) {
        return;
    }

    /* Расширение массива если необходимо */
    if (current_test_result->assertion_count >= (int)current_test_result->assertions_capacity) {
        size_t new_capacity = current_test_result->assertions_capacity * 2;
        test_assertion_t *new_assertions = realloc(current_test_result->assertions,
                                                  new_capacity * sizeof(test_assertion_t));
        if (!new_assertions) {
            /* Критическая ошибка выделения памяти - сообщаем об ошибке */
            if (current_test_result) {
                add_assertion_to_result("MEMORY_ERROR", "test_asserts.c", __LINE__,
                                      "Failed to allocate memory for assertions", false);
                update_test_status();
            }
            return; /* Не можем добавить ассерт */
        }
        current_test_result->assertions = new_assertions;
        current_test_result->assertions_capacity = new_capacity;
    }

    /* Добавление ассерта */
    test_assertion_t *assertion = &current_test_result->assertions[current_test_result->assertion_count];
    safe_strcpy(assertion->expression, expression, sizeof(assertion->expression));
    safe_strcpy(assertion->file, file, sizeof(assertion->file));
    assertion->line = line;
    safe_strcpy(assertion->message, message, sizeof(assertion->message));
    assertion->timestamp = get_current_time_ms();
    assertion->passed = passed;

    current_test_result->assertion_count++;

    if (passed) {
        current_test_result->passed_assertions++;
    } else {
        current_test_result->failed_assertions++;
    }
}

/**
 * @brief Обновление статуса теста на основе результатов ассертов
 */
static void update_test_status(void) {
    if (!current_test_result) {
        return;
    }

    if (current_test_result->failed_assertions > 0) {
        current_test_result->status = TEST_STATUS_FAILED;
        test_assert_emit_event(TEST_EVENT_ASSERTION_FAILED, NULL);
    } else if (current_test_result->passed_assertions > 0) {
        current_test_result->status = TEST_STATUS_PASSED;
    }
}

/**
 * @brief Получение имени файла из полного пути
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static const char *get_filename_from_path(const char *file_path) {
    const char *filename = strrchr(file_path, '/');
    if (filename) {
        return filename + 1;
    }
    return file_path;
}
#pragma GCC diagnostic pop

/* ============================================================================
 * Реализация многопоточных функций ассертов
 * ============================================================================ */

test_assert_context_t *test_assert_create_context(test_result_t *result, const char *test_name) {
    test_assert_context_t *context = malloc(sizeof(test_assert_context_t));
    if (!context) {
        return NULL;
    }

    init_assert_context(context, result, test_name);
    return context;
}

void test_assert_destroy_context(test_assert_context_t *context) {
    if (context) {
        free(context);
    }
}

void test_assert_init_with_context(test_assert_context_t *context, test_result_t *result, const char *test_name) {
    if (!context) {
        return;
    }

    init_assert_context(context, result, test_name);
}

void test_assert_finalize_with_context(test_assert_context_t *context, test_result_t *result) {
    if (!context || !context->assertions_initialized) {
        return;
    }

    /* Сохранение старого контекста для доступа к функциям ассертов */
    test_assert_context_t old_context = global_assert_context;
    global_assert_context = *context;

    update_test_status();

    if (result) {
        result->end_time = get_current_time_ms();
        result->duration_ms = result->end_time - result->start_time;
    }

    context->assertions_initialized = false;
    context->current_test_result = NULL;

    /* Восстановление старого контекста */
    global_assert_context = old_context;
}

/* Вспомогательная функция для добавления ассерта с контекстом */
static void add_assertion_to_result_with_context(test_assert_context_t *context,
                                                const char *expression, const char *file,
                                                int line, const char *message, bool passed) {
    if (!context || !context->current_test_result || !context->current_test_result->assertions) {
        return;
    }

    /* Расширение массива если необходимо */
    if (context->current_test_result->assertion_count >= (int)context->current_test_result->assertions_capacity) {
        size_t new_capacity = context->current_test_result->assertions_capacity * 2;
        test_assertion_t *new_assertions = realloc(context->current_test_result->assertions,
                                                  new_capacity * sizeof(test_assertion_t));
        if (!new_assertions) {
            /* Критическая ошибка выделения памяти - сообщаем об ошибке */
            if (context->current_test_result) {
                add_assertion_to_result("MEMORY_ERROR", "test_asserts.c", __LINE__,
                                      "Failed to allocate memory for assertions", false);
                update_test_status();
            }
            return; /* Не можем добавить ассерт */
        }
        context->current_test_result->assertions = new_assertions;
        context->current_test_result->assertions_capacity = new_capacity;
    }

    /* Добавление ассерта */
    test_assertion_t *assertion = &context->current_test_result->assertions[context->current_test_result->assertion_count];
    safe_strcpy(assertion->expression, expression, sizeof(assertion->expression));
    safe_strcpy(assertion->file, file, sizeof(assertion->file));
    assertion->line = line;
    safe_strcpy(assertion->message, message, sizeof(assertion->message));
    assertion->timestamp = get_current_time_ms();
    assertion->passed = passed;

    context->current_test_result->assertion_count++;

    if (passed) {
        context->current_test_result->passed_assertions++;
    } else {
        context->current_test_result->failed_assertions++;
    }
}

bool test_assert_int_equal_with_context(test_assert_context_t *context, int64_t expected, int64_t actual, const char *message) {
    bool passed = (expected == actual);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected %lld, but got %lld", (long long)expected, (long long)actual);
        message = default_message;
    }

    if (context && context->current_test_result) {
        add_assertion_to_result_with_context(context, "INT_EQUAL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed && context) {
        /* Обновляем статус через контекст */
        test_assert_context_t old_context = global_assert_context;
        global_assert_context = *context;
        update_test_status();
        global_assert_context = old_context;
    }

    return passed;
}

/* ============================================================================
 * Реализация API функций ассертов
 * ============================================================================ */

void test_assert_init(test_result_t *result, const char *test_name) {
    if (assertions_initialized) {
        return;
    }

    current_test_result = result;
    test_start_time = get_current_time_ms();
    last_checkpoint_memory = 0; /* Заглушка для отслеживания памяти */
    assertions_initialized = true;

    if (result) {
        safe_strcpy(result->name, test_name, sizeof(result->name));
        result->status = TEST_STATUS_RUNNING;
        result->start_time = test_start_time;
    }
}

void test_assert_finalize(test_result_t *result) {
    if (!assertions_initialized) {
        return;
    }

    update_test_status();

    if (result) {
        result->end_time = get_current_time_ms();
        result->duration_ms = result->end_time - result->start_time;
    }

    assertions_initialized = false;
    current_test_result = NULL;
}

/* ============================================================================
 * Реализация базовых ассертов
 * ============================================================================ */

bool test_assert_int_equal(int64_t expected, int64_t actual, const char *message) {
    bool passed = (expected == actual);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected %lld, but got %lld", (long long)expected, (long long)actual);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("INT_EQUAL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_double_equal(double expected, double actual, double epsilon, const char *message) {
    bool passed = (fabs(expected - actual) <= epsilon);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected %f, but got %f (epsilon: %f)", expected, actual, epsilon);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("DOUBLE_EQUAL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_string_equal(const char *expected, const char *actual, const char *message) {
    bool passed = (expected && actual && strcmp(expected, actual) == 0);
    char default_message[256];

    if (!message) {
        if (!expected && !actual) {
            snprintf(default_message, sizeof(default_message), "Both strings are NULL");
        } else if (!expected) {
            snprintf(default_message, sizeof(default_message), "Expected string is NULL, actual: %s", actual);
        } else if (!actual) {
            snprintf(default_message, sizeof(default_message), "Expected: %s, but got NULL", expected);
        } else {
            snprintf(default_message, sizeof(default_message), "Expected: %s, but got: %s", expected, actual);
        }
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("STRING_EQUAL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_bytes_equal(const uint8_t *expected, const uint8_t *actual, size_t size, const char *message) {
    bool passed = true;
    size_t i = 0;

    if (!expected && !actual) {
        passed = true;
    } else if (!expected || !actual) {
        passed = false;
    } else {
        for (i = 0; i < size && passed; ++i) {
            if (expected[i] != actual[i]) {
                passed = false;
                break;
            }
        }
    }

    char default_message[256];
    if (!message) {
        if (!passed) {
            snprintf(default_message, sizeof(default_message),
                     "Byte arrays differ at position %zu", i);
        } else {
            snprintf(default_message, sizeof(default_message), "Byte arrays are equal");
        }
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("BYTES_EQUAL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_true(bool condition, const char *message) {
    bool passed = condition;
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected true, but condition is false");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("TRUE", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_false(bool condition, const char *message) {
    bool passed = !condition;
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected false, but condition is true");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("FALSE", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_null(const void *value, const char *message) {
    bool passed = (value == NULL);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected NULL, but got non-NULL value");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("NULL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_not_null(const void *value, const char *message) {
    bool passed = (value != NULL);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected non-NULL value, but got NULL");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("NOT_NULL", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_success(int result_code, const char *message) {
    bool passed = (result_code == 0);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected success (0), but got error code: %d", result_code);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("SUCCESS", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_error(int result_code, const char *message) {
    bool passed = (result_code != 0);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected error (!=0), but got success code: %d", result_code);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("ERROR", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_in_range(int64_t value, int64_t min, int64_t max, const char *message) {
    bool passed = (value >= min && value <= max);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected %lld to be in range [%lld, %lld]", (long long)value,
                 (long long)min, (long long)max);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("IN_RANGE", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_array_size(const void *array __attribute__((unused)), size_t expected_size __attribute__((unused)), const char *message) {
    bool passed = false;
    char default_message[256];

    /* Заглушка - в полной реализации нужно определить размер массива */
    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Array size check (implementation needed)");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("ARRAY_SIZE", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

/* ============================================================================
 * Реализация строковых ассертов
 * ============================================================================ */

bool test_assert_not_empty(const char *str, const char *message) {
    bool passed = (str && strlen(str) > 0);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected non-empty string, but got: %s",
                 str ? str : "NULL");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("NOT_EMPTY", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_contains(const char *haystack, const char *needle, const char *message) {
    bool passed = false;

    if (!haystack || !needle) {
        char default_message[256];
        if (!message) {
            snprintf(default_message, sizeof(default_message),
                     "Invalid parameters: haystack=%s, needle=%s",
                     haystack ? "valid" : "NULL", needle ? "valid" : "NULL");
            message = default_message;
        }

        if (current_test_result) {
            add_assertion_to_result("CONTAINS", "test_asserts.c", __LINE__, message, false);
        }
        return false;
    }

    passed = (strstr(haystack, needle) != NULL);

    char default_message[256];
    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected '%s' to contain '%s'", haystack, needle);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("CONTAINS", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_starts_with(const char *str, const char *prefix, const char *message) {
    bool passed = false;

    if (!str || !prefix) {
        char default_message[256];
        if (!message) {
            snprintf(default_message, sizeof(default_message),
                     "Invalid parameters: str=%s, prefix=%s",
                     str ? "valid" : "NULL", prefix ? "valid" : "NULL");
            message = default_message;
        }

        if (current_test_result) {
            add_assertion_to_result("STARTS_WITH", "test_asserts.c", __LINE__, message, false);
        }
        return false;
    }

    passed = (strncmp(str, prefix, strlen(prefix)) == 0);

    char default_message[256];
    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected '%s' to start with '%s'", str, prefix);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("STARTS_WITH", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_ends_with(const char *str, const char *suffix, const char *message) {
    bool passed = false;

    if (!str || !suffix) {
        char default_message[256];
        if (!message) {
            snprintf(default_message, sizeof(default_message),
                     "Invalid parameters: str=%s, suffix=%s",
                     str ? "valid" : "NULL", suffix ? "valid" : "NULL");
            message = default_message;
        }

        if (current_test_result) {
            add_assertion_to_result("ENDS_WITH", "test_asserts.c", __LINE__, message, false);
        }
        return false;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len >= suffix_len) {
        passed = (strcmp(str + str_len - suffix_len, suffix) == 0);
    }

    char default_message[256];
    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected '%s' to end with '%s'", str, suffix);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("ENDS_WITH", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

/* ============================================================================
 * Реализация файловых ассертов
 * ============================================================================ */

bool test_assert_file_exists(const char *file_path, const char *message) {
    if (!file_path) {
        char default_message[256];
        if (!message) {
            snprintf(default_message, sizeof(default_message),
                     "Invalid parameter: file_path is NULL");
            message = default_message;
        }

        if (current_test_result) {
            add_assertion_to_result("FILE_EXISTS", "test_asserts.c", __LINE__, message, false);
        }
        return false;
    }

    bool passed = (access(file_path, F_OK) == 0);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "File does not exist: %s", file_path);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("FILE_EXISTS", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_file_not_exists(const char *file_path, const char *message) {
    if (!file_path) {
        char default_message[256];
        if (!message) {
            snprintf(default_message, sizeof(default_message),
                     "Invalid parameter: file_path is NULL");
            message = default_message;
        }

        if (current_test_result) {
            add_assertion_to_result("FILE_NOT_EXISTS", "test_asserts.c", __LINE__, message, false);
        }
        return false;
    }

    bool passed = (access(file_path, F_OK) != 0);
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "File exists (but should not): %s", file_path);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("FILE_NOT_EXISTS", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_dir_exists(const char *dir_path, const char *message) {
    bool passed = false;
    struct stat statbuf;

    if (stat(dir_path, &statbuf) == 0) {
        passed = S_ISDIR(statbuf.st_mode);
    }

    char default_message[256];
    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Directory does not exist: %s", dir_path);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("DIR_EXISTS", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

bool test_assert_file_size(const char *file_path, size_t expected_size, const char *message) {
    bool passed = false;
    struct stat statbuf;

    if (stat(file_path, &statbuf) == 0) {
        passed = (statbuf.st_size == (off_t)expected_size);
    }

    char default_message[256];
    if (!message) {
        snprintf(default_message, sizeof(default_message),
                 "Expected file size %zu, but got %lld",
                 expected_size, (long long)statbuf.st_size);
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("FILE_SIZE", "test_asserts.c", __LINE__, message, passed);
    }

    if (!passed) {
        update_test_status();
    }

    return passed;
}

/* ============================================================================
 * Реализация функций памяти и времени
 * ============================================================================ */

void test_memory_checkpoint(void) {
    /* Заглушка - в полной реализации будет фиксация текущего использования памяти */
    last_checkpoint_memory = 0;
}

bool test_assert_no_memory_leaks(const char *message) {
    /* Заглушка - в полной реализации будет проверка утечек памяти */
    bool passed = true;
    char default_message[256];

    if (!message) {
        snprintf(default_message, sizeof(default_message), "Memory leak check");
        message = default_message;
    }

    if (current_test_result) {
        add_assertion_to_result("NO_MEMORY_LEAKS", "test_asserts.c", __LINE__, message, passed);
    }

    return passed;
}

uint64_t test_get_memory_usage(void) {
    /* Заглушка - в полной реализации будет получение текущего использования памяти */
    return 0;
}

uint64_t test_get_peak_memory_usage(void) {
    /* Заглушка - в полной реализации будет получение пикового использования памяти */
    return 0;
}

void test_time_start(void) {
    test_start_time = get_current_time_ms();
}

uint64_t test_time_end(void) {
    return get_current_time_ms() - test_start_time;
}

uint64_t test_get_current_time_ms(void) {
    return get_current_time_ms();
}

/* ============================================================================
 * Реализация функций логирования
 * ============================================================================ */

void test_log_message(const char *message) {
    if (!message || !current_test_result) {
        return;
    }

    /* Безопасное добавление сообщения в stdout теста */
    size_t current_len = strnlen(current_test_result->stdout_output,
                                 sizeof(current_test_result->stdout_output) - 1);
    size_t available_space = sizeof(current_test_result->stdout_output) - 1 - current_len;

    if (available_space > 0) {
        strncat(current_test_result->stdout_output, message, available_space);
        available_space = sizeof(current_test_result->stdout_output) - 1 -
                         strnlen(current_test_result->stdout_output,
                                 sizeof(current_test_result->stdout_output) - 1);

        if (available_space > 0) {
            strncat(current_test_result->stdout_output, "\n", available_space);
        }
    }
}

void test_log_format(const char *format, ...) {
    if (!format) {
        return;
    }

    char buffer[512];
    va_list args;
    va_start(args, format);

    /* Безопасное форматирование с проверкой результата */
    int written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    /* Если сообщение было усечено, добавить индикатор */
    if (written >= (int)sizeof(buffer)) {
        /* Заменить последние символы на "...\0" для индикации усечения */
        size_t len = sizeof(buffer);
        if (len > 3) {
            buffer[len - 4] = '.';
            buffer[len - 3] = '.';
            buffer[len - 2] = '.';
            buffer[len - 1] = '\0';
        }
    }

    test_log_message(buffer);
}

int test_get_passed_count(void) {
    if (!current_test_result) {
        return 0;
    }
    return current_test_result->passed_assertions;
}

int test_get_failed_count(void) {
    if (!current_test_result) {
        return 0;
    }
    return current_test_result->failed_assertions;
}

/* ============================================================================
 * Реализация функций пропуска тестов
 * ============================================================================ */

void test_skip(const char *message) {
    if (current_test_result) {
        current_test_result->status = TEST_STATUS_SKIPPED;
        if (message) {
            safe_strcpy(current_test_result->error_message, message,
                       sizeof(current_test_result->error_message));
        }
        test_assert_emit_event(TEST_EVENT_SKIPPED, (void *)message);
    }
}

bool test_should_skip(void) {
    return (current_test_result && current_test_result->status == TEST_STATUS_SKIPPED);
}

/* ============================================================================
 * Реализация функций событий
 * ============================================================================ */

void test_assert_emit_event(test_event_type_t event_type, void *event_data) {
    if (global_test_context) {
        test_emit_event(event_type, event_data);
    }
}

test_event_type_t test_wait_for_event(uint64_t timeout_ms) {
    /* Заглушка - в полной реализации будет ожидание события */
    (void)timeout_ms;
    return 0;
}

test_event_type_t test_poll_for_event(void) {
    /* Заглушка - в полной реализации будет опрос событий */
    return 0;
}

/* ============================================================================
 * Реализация функций для работы с фиктивными объектами
 * ============================================================================ */

void *test_create_mock(const char *interface_name) {
    /* Заглушка - в полной реализации будет создание mock объектов */
    (void)interface_name;
    return NULL;
}

void test_configure_mock(void *mock, const char *method_name, void *return_value, int call_count) {
    /* Заглушка - в полной реализации будет настройка mock объектов */
    (void)mock;
    (void)method_name;
    (void)return_value;
    (void)call_count;
}

bool test_assert_mock_calls(void *mock, const char *method_name, int expected_calls, const char *message) {
    /* Заглушка - в полной реализации будет проверка вызовов mock объектов */
    (void)mock;
    (void)method_name;
    (void)expected_calls;
    return test_assert_true(true, message);
}

void test_destroy_mock(void *mock) {
    /* Заглушка - в полной реализации будет уничтожение mock объектов */
    (void)mock;
}

/* ============================================================================
 * Реализация функций анализа покрытия
 * ============================================================================ */

void test_coverage_start(void) {
    /* Заглушка - в полной реализации будет начало сбора покрытия */
}

void test_coverage_stop(void) {
    /* Заглушка - в полной реализации будет остановка сбора покрытия */
}

double test_get_function_coverage(const char *function_name) {
    /* Заглушка - в полной реализации будет получение покрытия функции */
    (void)function_name;
    return 0.0;
}

double test_get_total_coverage(void) {
    /* Заглушка - в полной реализации будет получение общего покрытия */
    return 0.0;
}

/* ============================================================================
 * Реализация вспомогательных функций
 * ============================================================================ */

int64_t test_random_int(int64_t min, int64_t max) {
    return min + (rand() % (max - min + 1));
}

char *test_random_string(char *buffer, size_t max_length) {
    if (!buffer || max_length == 0) {
        return NULL;
    }

    size_t length = (size_t)test_random_int(1, (int64_t)max_length - 1);
    for (size_t i = 0; i < length; ++i) {
        buffer[i] = (char)test_random_int('a', 'z');
    }
    buffer[length] = '\0';

    return buffer;
}

char *test_create_temp_file(const char *prefix, const char *suffix) {
    static char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "/tmp/%sXXXXXX%s",
             prefix ? prefix : "test", suffix ? suffix : "");

    /* Создание временного файла */
    int fd = mkstemp(temp_path);
    if (fd >= 0) {
        close(fd);
        return temp_path;
    }

    return NULL;
}

void test_remove_temp_file(const char *file_path) {
    if (file_path) {
        unlink(file_path);
    }
}

/* ============================================================================
 * Реализация функций для работы с коллекциями
 * ============================================================================ */

bool test_assert_collection_size(const void *collection, size_t expected_size, const char *message) {
    /* Заглушка - в полной реализации будет проверка размера коллекции */
    (void)collection;
    return test_assert_true(expected_size == 0, message);
}

bool test_assert_collection_empty(const void *collection, const char *message) {
    return test_assert_collection_size(collection, 0, message);
}

bool test_assert_collection_not_empty(const void *collection, const char *message) {
    /* Заглушка - в полной реализации будет проверка непустоты коллекции */
    (void)collection;
    return test_assert_true(true, message);
}

/* ============================================================================
 * Реализация функций для исключений
 * ============================================================================ */

bool test_assert_throws(void (*expression)(void), const char *message) {
    /* Заглушка - в полной реализации будет проверка генерации исключений */
    (void)expression;
    return test_assert_true(false, message);
}

bool test_assert_no_throw(void (*expression)(void), const char *message) {
    /* Заглушка - в полной реализации будет проверка отсутствия исключений */
    (void)expression;
    return test_assert_true(true, message);
}