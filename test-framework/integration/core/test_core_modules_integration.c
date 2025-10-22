// test_core_modules.c - Тесты для проверки основных модулей системы мониторинга
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "src/system_info.h"
#include "src/processes.h"
#include "src/network.h"
#include "src/disk.h"
#include "src/logging.h"
#include "src/battery.h"
#include "src/gpu.h"
#include "src/developer.h"

// Простой тестовый фреймворк
#define TEST_ASSERT(condition, message) do { \
    if (!(condition)) { \
        log_error("TEST FAILED: " message); \
        return -1; \
    } \
} while(0)

#define TEST_RUN(name) do { \
    log_info("Running " #name "..."); \
    if (test_##name() == 0) { \
        log_info("✓ " #name " PASSED"); \
    } else { \
        log_error("✗ " #name " FAILED"); \
        return -1; \
    } \
} while(0)

int test_system_info() {
    log_info("Testing system_info module...");

    // Тестируем CPU usage
    float cpu = get_cpu_usage();
    TEST_ASSERT(cpu >= 0.0f && cpu <= 100.0f, "CPU usage should be 0-100%");

    // Тестируем memory usage
    float mem = get_memory_usage();
    TEST_ASSERT(mem >= 0.0f && mem <= 100.0f, "Memory usage should be 0-100%");

    // Тестируем uptime
    long uptime = get_uptime_seconds();
    TEST_ASSERT(uptime >= 0, "Uptime should be non-negative");

    // Тестируем CPU frequencies
    long long curr_hz, max_hz;
    int freq_result = get_cpu_frequencies(&curr_hz, &max_hz);
    TEST_ASSERT(freq_result == 0, "get_cpu_frequencies should succeed");

    // Тестируем per-core usage
    float cores[32];
    int written;
    int cores_result = get_per_core_usage(cores, 32, &written);
    TEST_ASSERT(cores_result == 0, "get_per_core_usage should succeed");

    // Тестируем memory breakdown
    memory_breakdown_t mb;
    int mb_result = get_memory_breakdown(&mb);
    TEST_ASSERT(mb_result == 0, "get_memory_breakdown should succeed");

    return 0;
}

int test_processes() {
    log_info("Testing processes module...");

    // Тестируем получение списка процессов
    process_info_t procs[100];
    size_t count = get_process_list(procs, 100);
    TEST_ASSERT(count > 0, "Should find at least one process");
    TEST_ASSERT(count <= 100, "Should not exceed buffer size");

    // Проверяем корректность данных процессов
    for (size_t i = 0; i < count && i < 10; i++) {
        TEST_ASSERT(procs[i].pid > 0, "Process PID should be positive");
        TEST_ASSERT(strlen(procs[i].name) > 0, "Process name should not be empty");
        TEST_ASSERT(procs[i].cpu_usage >= 0.0f && procs[i].cpu_usage <= 100.0f, "CPU usage should be 0-100%");
        TEST_ASSERT(procs[i].mem_usage >= 0.0f && procs[i].mem_usage <= 100.0f, "Memory usage should be 0-100%");
    }

    return 0;
}

int test_network() {
    log_info("Testing network module...");

    // Тестируем получение сетевой информации
    const char *net_info = get_network_info();
    TEST_ASSERT(net_info != NULL, "Network info should not be NULL");
    TEST_ASSERT(strlen(net_info) > 0, "Network info should not be empty");

    // Тестируем получение RX/TX скоростей
    float rx = get_network_rx();
    float tx = get_network_tx();
    TEST_ASSERT(rx >= 0.0f, "RX rate should be non-negative");
    TEST_ASSERT(tx >= 0.0f, "TX rate should be non-negative");

    return 0;
}

int test_disk() {
    log_info("Testing disk module...");

    // Тестируем получение информации о диске
    const char *disk_info = get_disk_info();
    TEST_ASSERT(disk_info != NULL, "Disk info should not be NULL");
    TEST_ASSERT(strlen(disk_info) > 0, "Disk info should not be empty");

    // Тестируем получение использования диска для корневой файловой системы
    float usage = get_disk_usage("/");
    TEST_ASSERT(usage >= 0.0f && usage <= 100.0f, "Disk usage should be 0-100%");

    // Тестируем с некорректным путем
    float invalid_usage = get_disk_usage("/nonexistent/path");
    TEST_ASSERT(invalid_usage == 0.0f, "Invalid path should return 0%");

    return 0;
}

int test_logging() {
    log_info("Testing logging module...");

    // Тестируем базовое логирование (просто проверяем что не падает)
    log_info("Test info message");
    log_error("Test error message");

    return 0;
}

int test_battery() {
    log_info("Testing battery module...");

    battery_info_t bat;
    int result = get_battery_info(&bat);

    // Не обязательно должна быть батарея, но функция не должна падать
    TEST_ASSERT(result == 0, "get_battery_info should not crash");

    return 0;
}

int test_gpu() {
    log_info("Testing GPU module...");

    gpu_info_t gpu;
    int result = get_gpu_info(&gpu);

    // Функция должна работать без ошибок
    TEST_ASSERT(result == 0, "get_gpu_info should not crash");

    return 0;
}

int test_developer() {
    log_info("Testing developer functions...");

    // Тест портов
    port_info_t ports[10];
    int port_count;
    int result = get_listening_ports(ports, 10, &port_count);
    TEST_ASSERT(result == 0, "get_listening_ports should work");

    // Тест окружения разработки
    dev_environment_t env;
    result = get_dev_environment(&env);
    TEST_ASSERT(result == 0, "get_dev_environment should work");

    // Тест процессов разработки
    dev_process_t processes[10];
    int proc_count;
    result = get_dev_processes(processes, 10, &proc_count);
    TEST_ASSERT(result == 0, "get_dev_processes should work");

    return 0;
}

int main() {
    log_info("Starting core modules test suite...");

    TEST_RUN(logging);
    TEST_RUN(system_info);
    TEST_RUN(processes);
    TEST_RUN(network);
    TEST_RUN(disk);
    TEST_RUN(battery);
    TEST_RUN(gpu);
    TEST_RUN(developer);

    log_info("All tests PASSED! ✓");
    return 0;
}