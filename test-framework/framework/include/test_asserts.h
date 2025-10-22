/**
 * @file test_asserts.h
 * @brief Ассершены для тестового фреймворка
 *
 * Этот файл содержит функции ассертов, интегрированные с системой
 * событий и метрик производительности архитектуры SystemMonitor.
 */

#ifndef TEST_ASSERTS_H
#define TEST_ASSERTS_H

#include "test_framework.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

/* ============================================================================
 * Основные функции ассертов
 * ============================================================================ */

/**
 * @brief Инициализация системы ассертов для теста
 * @param result Структура результата теста
 * @param test_name Имя теста
 */
void test_assert_init(test_result_t *result, const char *test_name);

/**
 * @brief Финализация системы ассертов для теста
 * @param result Структура результата теста
 */
void test_assert_finalize(test_result_t *result);

/* ============================================================================
 * Многопоточные функции ассертов (thread-safe)
 * ============================================================================ */

/**
 * @brief Структура контекста ассертов для многопоточности
 */
typedef struct test_assert_context_struct {
    test_result_t *current_test_result;
    uint64_t test_start_time;
    uint64_t last_checkpoint_memory;
    bool assertions_initialized;
} test_assert_context_t;

/**
 * @brief Создание нового контекста ассертов
 * @param result Структура результата теста
 * @param test_name Имя теста
 * @return Указатель на созданный контекст или NULL при ошибке
 */
test_assert_context_t *test_assert_create_context(test_result_t *result, const char *test_name);

/**
 * @brief Уничтожение контекста ассертов
 * @param context Контекст для уничтожения
 */
void test_assert_destroy_context(test_assert_context_t *context);

/**
 * @brief Инициализация системы ассертов с контекстом
 * @param context Контекст ассертов
 * @param result Структура результата теста
 * @param test_name Имя теста
 */
void test_assert_init_with_context(test_assert_context_t *context, test_result_t *result, const char *test_name);

/**
 * @brief Финализация системы ассертов с контекстом
 * @param context Контекст ассертов
 * @param result Структура результата теста
 */
void test_assert_finalize_with_context(test_assert_context_t *context, test_result_t *result);

/**
 * @brief Многопоточная версия проверки равенства целых чисел
 * @param context Контекст ассертов
 * @param expected Ожидаемое значение
 * @param actual Фактическое значение
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_int_equal_with_context(test_assert_context_t *context, int64_t expected, int64_t actual, const char *message);

/* ============================================================================
 * Ассерты для сравнения значений
 * ============================================================================ */

/**
 * @brief Проверка равенства целых чисел
 * @param expected Ожидаемое значение
 * @param actual Фактическое значение
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_int_equal(int64_t expected, int64_t actual, const char *message);

/**
 * @brief Проверка равенства чисел с плавающей точкой
 * @param expected Ожидаемое значение
 * @param actual Фактическое значение
 * @param epsilon Допустимая погрешность
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_double_equal(double expected, double actual, double epsilon, const char *message);

/**
 * @brief Проверка равенства строк
 * @param expected Ожидаемая строка
 * @param actual Фактическая строка
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_string_equal(const char *expected, const char *actual, const char *message);

/**
 * @brief Проверка равенства массивов байт
 * @param expected Ожидаемый массив
 * @param actual Фактический массив
 * @param size Размер массивов
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_bytes_equal(const uint8_t *expected, const uint8_t *actual, size_t size, const char *message);

/* ============================================================================
 * Ассерты для проверки условий
 * ============================================================================ */

/**
 * @brief Проверка истинности условия
 * @param condition Условие для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_true(bool condition, const char *message);

/**
 * @brief Проверка ложности условия
 * @param condition Условие для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_false(bool condition, const char *message);

/**
 * @brief Проверка на NULL
 * @param value Значение для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_null(const void *value, const char *message);

/**
 * @brief Проверка на не-NULL
 * @param value Значение для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_not_null(const void *value, const char *message);

/* ============================================================================
 * Ассерты для работы с памятью
 * ============================================================================ */

/**
 * @brief Проверка успешного выполнения операции (возврат 0)
 * @param result_code Код результата операции
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_success(int result_code, const char *message);

/**
 * @brief Проверка ошибки выполнения операции (возврат != 0)
 * @param result_code Код результата операции
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_error(int result_code, const char *message);

/**
 * @brief Проверка принадлежности значения диапазону
 * @param value Значение для проверки
 * @param min Минимальное значение диапазона
 * @param max Максимальное значение диапазона
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_in_range(int64_t value, int64_t min, int64_t max, const char *message);

/**
 * @brief Проверка размера массива
 * @param array Массив для проверки
 * @param expected_size Ожидаемый размер
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_array_size(const void *array, size_t expected_size, const char *message);

/* ============================================================================
 * Ассерты для работы со строками
 * ============================================================================ */

/**
 * @brief Проверка непустой строки
 * @param str Строка для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_not_empty(const char *str, const char *message);

/**
 * @brief Проверка содержимого строки в другой строке
 * @param haystack Строка, в которой производится поиск
 * @param needle Подстрока для поиска
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_contains(const char *haystack, const char *needle, const char *message);

/**
 * @brief Проверка начала строки
 * @param str Строка для проверки
 * @param prefix Префикс для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_starts_with(const char *str, const char *prefix, const char *message);

/**
 * @brief Проверка окончания строки
 * @param str Строка для проверки
 * @param suffix Суффикс для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_ends_with(const char *str, const char *suffix, const char *message);

/* ============================================================================
 * Ассерты для работы с файлами и файловой системой
 * ============================================================================ */

/**
 * @brief Проверка существования файла
 * @param file_path Путь к файлу
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_file_exists(const char *file_path, const char *message);

/**
 * @brief Проверка отсутствия файла
 * @param file_path Путь к файлу
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_file_not_exists(const char *file_path, const char *message);

/**
 * @brief Проверка существования директории
 * @param dir_path Путь к директории
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_dir_exists(const char *dir_path, const char *message);

/**
 * @brief Проверка размера файла
 * @param file_path Путь к файлу
 * @param expected_size Ожидаемый размер
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_file_size(const char *file_path, size_t expected_size, const char *message);

/* ============================================================================
 * Ассерты для работы с коллекциями
 * ============================================================================ */

/**
 * @brief Проверка размера коллекции
 * @param collection Коллекция для проверки
 * @param expected_size Ожидаемый размер
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_collection_size(const void *collection, size_t expected_size, const char *message);

/**
 * @brief Проверка пустоты коллекции
 * @param collection Коллекция для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_collection_empty(const void *collection, const char *message);

/**
 * @brief Проверка непустоты коллекции
 * @param collection Коллекция для проверки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_collection_not_empty(const void *collection, const char *message);

/* ============================================================================
 * Ассерты для работы с исключениями/ошибками
 * ============================================================================ */

/**
 * @brief Проверка генерации исключения
 * @param expression Выражение, которое должно генерировать исключение
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_throws(void (*expression)(void), const char *message);

/**
 * @brief Проверка отсутствия исключений
 * @param expression Выражение, которое не должно генерировать исключений
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_no_throw(void (*expression)(void), const char *message);

/* ============================================================================
 * Макросы для удобного использования ассертов
 * ============================================================================ */

/**
 * @brief Макрос для базового ассерта с автоматической регистрацией
 * @param condition Условие для проверки
 * @param message Сообщение об ошибке
 */
#define TEST_ASSERT_BASE(condition, message) \
    do { \
        if (!test_assert_true(condition, message)) { \
            return; \
        } \
    } while(0)

/**
 * @brief Макросы для ассертов с автоматическим сообщением
 */
#define TEST_ASSERT_TRUE_WITH_MSG(condition, message) \
    TEST_ASSERT_BASE(condition, message)

#define TEST_ASSERT_FALSE_WITH_MSG(condition, message) \
    TEST_ASSERT_BASE(!condition, message)

#define TEST_ASSERT_NULL_WITH_MSG(value, message) \
    TEST_ASSERT_BASE(value == NULL, message)

#define TEST_ASSERT_NOT_NULL_WITH_MSG(value, message) \
    TEST_ASSERT_BASE(value != NULL, message)

#define TEST_ASSERT_SUCCESS_WITH_MSG(expression, message) \
    TEST_ASSERT_BASE(expression == 0, message)

#define TEST_ASSERT_ERROR_WITH_MSG(expression, message) \
    TEST_ASSERT_BASE(expression != 0, message)

#define TEST_ASSERT_EQUAL_WITH_MSG(expected, actual, message) \
    do { \
        if (!test_assert_int_equal(expected, actual, message)) { \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_DOUBLE_EQUAL_WITH_MSG(expected, actual, message) \
    do { \
        if (!test_assert_double_equal(expected, actual, 0.0001, message)) { \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_STRING_EQUAL_WITH_MSG(expected, actual, message) \
    do { \
        if (!test_assert_string_equal(expected, actual, message)) { \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_IN_RANGE_WITH_MSG(value, min, max, message) \
    do { \
        if (!test_assert_in_range(value, min, max, message)) { \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_ARRAY_SIZE_WITH_MSG(array, expected_size, message) \
    do { \
        if (!test_assert_array_size(array, expected_size, message)) { \
            return; \
        } \
    } while(0)

/* ============================================================================
 * Функции для работы с памятью и производительностью
 * ============================================================================ */

/**
 * @brief Установка точки отслеживания памяти
 */
void test_memory_checkpoint(void);

/**
 * @brief Проверка отсутствия утечек памяти с предыдущей точки
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_no_memory_leaks(const char *message);

/**
 * @brief Получение текущего использования памяти
 * @return Количество используемой памяти в байтах
 */
uint64_t test_get_memory_usage(void);

/**
 * @brief Получение пикового использования памяти
 * @return Пиковое использование памяти в байтах
 */
uint64_t test_get_peak_memory_usage(void);

/* ============================================================================
 * Функции для работы со временем
 * ============================================================================ */

/**
 * @brief Запуск таймера для замера времени выполнения
 */
void test_time_start(void);

/**
 * @brief Остановка таймера и получение затраченного времени
 * @return Затраченное время в миллисекундах
 */
uint64_t test_time_end(void);

/**
 * @brief Получение текущего времени в миллисекундах
 * @return Текущее время в миллисекундах
 */
uint64_t test_get_current_time_ms(void);

/* ============================================================================
 * Функции для логирования и диагностики
 * ============================================================================ */

/**
 * @brief Логирование сообщения в результат теста
 * @param message Сообщение для логирования
 */
void test_log_message(const char *message);

/**
 * @brief Логирование форматированного сообщения
 * @param format Формат строки (как в printf)
 * @param ... Аргументы
 */
void test_log_format(const char *format, ...);

/**
 * @brief Получение количества пройденных ассертов
 * @return Количество пройденных ассертов
 */
int test_get_passed_count(void);

/**
 * @brief Получение количества проваленных ассертов
 * @return Количество проваленных ассертов
 */
int test_get_failed_count(void);

/* ============================================================================
 * Функции для пропуска тестов
 * ============================================================================ */

/**
 * @brief Пропуск теста с сообщением
 * @param message Причина пропуска
 */
void test_skip(const char *message);

/**
 * @brief Проверка необходимости пропуска теста
 * @return true если тест должен быть пропущен
 */
bool test_should_skip(void);

/* ============================================================================
 * Функции для работы с событиями в тестах
 * ============================================================================ */

/**
 * @brief Отправка события из теста (внутренняя функция ассертов)
 * @param event_type Тип события
 * @param event_data Данные события
 */
void test_assert_emit_event(test_event_type_t event_type, void *event_data);

/**
 * @brief Ожидание события с таймаутом
 * @param timeout_ms Таймаут в миллисекундах
 * @return Тип полученного события или 0 если таймаут
 */
test_event_type_t test_wait_for_event(uint64_t timeout_ms);

/**
 * @brief Опрос событий без блокировки
 * @return Тип полученного события или 0 если событий нет
 */
test_event_type_t test_poll_for_event(void);

/* ============================================================================
 * Функции для работы с фиктивными объектами (mocks)
 * ============================================================================ */

/**
 * @brief Создание фиктивного объекта с заданным поведением
 * @param interface_name Имя интерфейса для имитации
 * @return Указатель на фиктивный объект
 */
void *test_create_mock(const char *interface_name);

/**
 * @brief Настройка поведения фиктивного объекта
 * @param mock Указатель на фиктивный объект
 * @param method_name Имя метода
 * @param return_value Возвращаемое значение
 * @param call_count Ожидаемое количество вызовов
 */
void test_configure_mock(void *mock, const char *method_name, void *return_value, int call_count);

/**
 * @brief Проверка вызовов фиктивного объекта
 * @param mock Указатель на фиктивный объект
 * @param method_name Имя метода
 * @param expected_calls Ожидаемое количество вызовов
 * @param message Сообщение об ошибке
 * @return true при успехе, false при ошибке
 */
bool test_assert_mock_calls(void *mock, const char *method_name, int expected_calls, const char *message);

/**
 * @brief Уничтожение фиктивного объекта
 * @param mock Указатель на фиктивный объект
 */
void test_destroy_mock(void *mock);

/* ============================================================================
 * Функции для анализа покрытия кода
 * ============================================================================ */

/**
 * @brief Начало сбора данных о покрытии
 */
void test_coverage_start(void);

/**
 * @brief Остановка сбора данных о покрытии
 */
void test_coverage_stop(void);

/**
 * @brief Получение покрытия для функции
 * @param function_name Имя функции
 * @return Процент покрытия (0-100)
 */
double test_get_function_coverage(const char *function_name);

/**
 * @brief Получение общего покрытия кода
 * @return Процент покрытия (0-100)
 */
double test_get_total_coverage(void);

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

/**
 * @brief Генерация случайного числа в диапазоне
 * @param min Минимальное значение
 * @param max Максимальное значение
 * @return Случайное число
 */
int64_t test_random_int(int64_t min, int64_t max);

/**
 * @brief Генерация случайной строки
 * @param buffer Буфер для сохранения строки
 * @param max_length Максимальная длина строки
 * @return Указатель на сгенерированную строку
 */
char *test_random_string(char *buffer, size_t max_length);

/**
 * @brief Создание временного файла
 * @param prefix Префикс имени файла
 * @param suffix Суффикс имени файла
 * @return Путь к созданному файлу или NULL при ошибке
 */
char *test_create_temp_file(const char *prefix, const char *suffix);

/**
 * @brief Удаление временного файла
 * @param file_path Путь к файлу
 */
void test_remove_temp_file(const char *file_path);

/* ============================================================================
 * Макросы для обратной совместимости
 * ============================================================================ */

#define ASSERT_TRUE TEST_ASSERT_TRUE
#define ASSERT_FALSE TEST_ASSERT_FALSE
#define ASSERT_EQUAL TEST_ASSERT_EQUAL
#define ASSERT_NULL TEST_ASSERT_NULL
#define ASSERT_NOT_NULL TEST_ASSERT_NOT_NULL
#define ASSERT_SUCCESS TEST_ASSERT_SUCCESS

/* ============================================================================
 * Макросы для работы с временем и событиями
 * ============================================================================ */

#define TEST_ASSERT_INIT(result, test_name) test_assert_init(result, test_name)
#define TEST_TIME_START() test_time_start()
#define TEST_TIME_END() test_time_end()
#define TEST_TIME_ELAPSED() test_time_end()
#define TEST_EMIT_EVENT(event_type, event_data) test_assert_emit_event(event_type, event_data)
#define TEST_ASSERT_FINALIZE(result) test_assert_finalize(result)
#define TEST_DIAGNOSTIC(format, ...) test_log_format(format, ##__VA_ARGS__)
#define TEST_LOG_MESSAGE(message) test_log_message(message)
#define TEST_SKIP(reason) test_skip(reason)
#define TEST_MEMORY_CHECKPOINT() test_memory_checkpoint()
#define TEST_GET_MEMORY_USAGE() test_get_memory_usage()

/* ============================================================================
 * Макросы для совместимости с другими тестовыми фреймворками
 * ============================================================================ */

#define TEST_ASSERT_INIT_WITH_MSG(result, test_name, msg) test_assert_init(result, test_name)
#define TEST_ASSERT_FINALIZE_WITH_MSG(result, msg) test_assert_finalize(result)

#endif // TEST_ASSERTS_H