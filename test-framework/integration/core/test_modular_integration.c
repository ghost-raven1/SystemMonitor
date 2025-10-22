/**
 * @file test_modular_simple.c
 * @brief Простые тесты модульной архитектуры без зависимостей от ncurses
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Тестирование базовых функций без зависимостей от UI
extern float get_cpu_usage(void);
extern float get_memory_usage(void);
extern long get_uptime_seconds(void);
extern int get_cpu_frequencies(long long *current_hz, long long *max_hz);

// Структура для тестирования
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} simple_test_results_t;

static simple_test_results_t g_results = {0, 0, 0};

// Макросы для тестирования
#define TEST_START(name) printf("🧪 Тестирование: %s...\n", name)
#define TEST_PASS(name) do { g_results.passed_tests++; g_results.total_tests++; printf("✅ ПАС: %s\n", name); } while(0)
#define TEST_FAIL(name) do { g_results.failed_tests++; g_results.total_tests++; printf("❌ НЕУДАЧА: %s\n", name); } while(0)

/**
 * @brief Тест получения использования CPU
 */
void test_cpu_usage(void) {
    TEST_START("Получение использования CPU");

    float cpu = get_cpu_usage();
    if (cpu >= 0.0f && cpu <= 100.0f) {
        printf("   CPU: %.2f%%\n", cpu);
        TEST_PASS("Получение использования CPU");
    } else {
        printf("   CPU: %.2f%% (недопустимое значение)\n", cpu);
        TEST_FAIL("Получение использования CPU");
    }
}

/**
 * @brief Тест получения использования памяти
 */
void test_memory_usage(void) {
    TEST_START("Получение использования памяти");

    float mem = get_memory_usage();
    if (mem >= 0.0f && mem <= 100.0f) {
        printf("   Память: %.2f%%\n", mem);
        TEST_PASS("Получение использования памяти");
    } else {
        printf("   Память: %.2f%% (недопустимое значение)\n", mem);
        TEST_FAIL("Получение использования памяти");
    }
}

/**
 * @brief Тест получения времени работы системы
 */
void test_uptime(void) {
    TEST_START("Получение времени работы системы");

    long uptime = get_uptime_seconds();
    if (uptime >= 0) {
        printf("   Время работы: %ld сек\n", uptime);
        TEST_PASS("Получение времени работы системы");
    } else {
        printf("   Время работы: %ld сек (недопустимое значение)\n", uptime);
        TEST_FAIL("Получение времени работы системы");
    }
}

/**
 * @brief Тест получения частот CPU
 */
void test_cpu_frequencies(void) {
    TEST_START("Получение частот CPU");

    long long current_hz, max_hz;
    int result = get_cpu_frequencies(&current_hz, &max_hz);

    if (result == 0 && current_hz > 0) {
        printf("   Текущая частота: %.2f GHz\n", (double)current_hz / 1e9);
        if (max_hz > 0) {
            printf("   Максимальная частота: %.2f GHz\n", (double)max_hz / 1e9);
        }
        TEST_PASS("Получение частот CPU");
    } else {
        printf("   Не удалось получить частоты CPU (код ошибки: %d)\n", result);
        TEST_FAIL("Получение частот CPU");
    }
}

/**
 * @brief Тест производительности получения метрик
 */
void test_performance(void) {
    TEST_START("Производительность получения метрик");

    clock_t start, end;
    double cpu_time_used;

    // Тестируем время получения CPU usage
    start = clock();
    for (int i = 0; i < 1000; i++) {
        get_cpu_usage();
    }
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("   1000 вызовов get_cpu_usage(): %.4f сек\n", cpu_time_used);

    if (cpu_time_used < 1.0) { // Должно быть быстро
        TEST_PASS("Производительность получения метрик");
    } else {
        TEST_FAIL("Производительность получения метрик");
    }
}

/**
 * @brief Основная функция тестирования
 */
int main(void) {
    printf("🚀 ПРОСТЫЕ ТЕСТЫ МОДУЛЬНОЙ АРХИТЕКТУРЫ\n");
    printf("=====================================\n\n");

    // Инициализируем результаты
    memset(&g_results, 0, sizeof(simple_test_results_t));

    // Запускаем тесты
    test_cpu_usage();
    test_memory_usage();
    test_uptime();
    test_cpu_frequencies();
    test_performance();

    // Выводим результаты
    printf("\n=====================================\n");
    printf("📊 РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ\n");
    printf("=====================================\n");
    printf("Всего тестов: %d\n", g_results.total_tests);
    printf("Пройдено: %d\n", g_results.passed_tests);
    printf("Неудачно: %d\n", g_results.failed_tests);
    printf("Успешность: %.1f%%\n", (float)g_results.passed_tests / g_results.total_tests * 100.0f);

    if (g_results.failed_tests == 0) {
        printf("\n🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!\n");
        printf("Базовые функции модульной архитектуры работают корректно.\n");
        return 0;
    } else {
        printf("\n❌ НЕКОТОРЫЕ ТЕСТЫ НЕ ПРОШЛИ\n");
        printf("Необходимо устранить проблемы в базовых функциях.\n");
        return 1;
    }
}