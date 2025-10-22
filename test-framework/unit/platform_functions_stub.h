
/**
 * @file platform_functions_stub.h
 * @brief Заглушки функций платформенных модулей для тестирования
 *
 * Этот файл содержит заглушки функций, которые используются в тестах,
 * но не реализованы в базовой версии платформенных модулей.
 */

#ifndef PLATFORM_FUNCTIONS_STUB_H
#define PLATFORM_FUNCTIONS_STUB_H

#include "../framework/include/system_monitor_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

/* ============================================================================
 * Заглушки функций платформенных модулей
 * ============================================================================ */

/**
 * @brief Получение типа платформы (заглушка)
 */
static inline int get_platform_type(void) {
    return PLATFORM_LINUX; /* Заглушка - возвращает Linux */
}

/**
 * @brief Получение информации о платформе (заглушка)
 */
static inline int get_platform_info(platform_info_t *info) {
    if (!info) return -1;

    safe_strcpy(info->platform_name, "Linux", sizeof(info->platform_name));
    safe_strcpy(info->version, "5.4.0", sizeof(info->version));
    safe_strcpy(info->architecture, "x86_64", sizeof(info->architecture));
    info->cpu_cores = 4;
    info->total_memory = 8ULL * 1024 * 1024 * 1024; /* 8 GB */
    safe_strcpy(info->kernel_version, "5.4.0-generic", sizeof(info->kernel_version));

    return 0;
}

/**
 * @brief Получение списка процессов (заглушка)
 */
static inline size_t get_process_list(process_info_t *processes, size_t max_count) {
    if (!processes || max_count == 0) return 0;

    size_t count = 0;

    /* Создание фиктивных процессов для тестирования */
    for (size_t i = 0; i < max_count && i < 5; i++) {
        processes[i].pid = 1000 + (int)i;
        snprintf(processes[i].name, sizeof(processes[i].name), "test_process_%zu", i);
        safe_strcpy(processes[i].user, "testuser", sizeof(processes[i].user));
        processes[i].cpu_usage = 10.0f + i * 5.0f;
        processes[i].mem_usage = 5.0f + i * 2.0f;
        processes[i].memory_bytes = (100 + i * 50) * 1024 * 1024;
        processes[i].start_time = 1234567890 + i * 3600;
        safe_strcpy(processes[i].state, "R", sizeof(processes[i].state));
        count++;
    }

    return count;
}

/**
 * @brief Получение процесса по PID (заглушка)
 */
static inline int get_process_by_pid(pid_t pid, process_info_t *process) {
    if (!process || pid <= 0) return -1;

    process->pid = pid;
    snprintf(process->name, sizeof(process->name), "process_%d", pid);
    safe_strcpy(process->user, "testuser", sizeof(process->user));
    process->cpu_usage = 15.0f;
    process->mem_usage = 8.0f;
    process->memory_bytes = 128 * 1024 * 1024;
    process->start_time = 1234567890;
    safe_strcpy(process->state, "R", sizeof(process->state));

    return 0;
}

/**
 * @brief Получение сетевой информации (заглушка)
 */
static inline const char *get_network_info(void) {
    static char info[256];
    snprintf(info, sizeof(info), "Network: eth0 (192.168.1.100), wlan0 (WiFi)");
    return info;
}

/**
 * @brief Получение RX скорости сети (заглушка)
 */
static inline float get_network_rx(void) {
    return 1024.5f; /* KB/s */
}

/**
 * @brief Получение TX скорости сети (заглушка)
 */
static inline float get_network_tx(void) {
    return 512.3f; /* KB/s */
}

/**
 * @brief Получение информации о диске (заглушка)
 */
static inline const char *get_disk_info(void) {
    static char info[256];
    snprintf(info, sizeof(info), "Disk: sda1 (SSD 500GB), sdb1 (HDD 1TB)");
    return info;
}

/**
 * @brief Получение использования диска (заглушка)
 */
static inline float get_disk_usage(const char *path) {
    if (!path) return 0.0f;

    if (strcmp(path, "/") == 0) {
        return 45.7f; /* Процент использования корневого диска */
    }

    return 0.0f;
}

/**
 * @brief Получение информации о GPU (заглушка)
 */
static inline int get_gpu_info(gpu_info_t *gpu) {
    if (!gpu) return -1;

    safe_strcpy(gpu->name, "NVIDIA GeForce GTX 1660", sizeof(gpu->name));
    safe_strcpy(gpu->driver, "nvidia", sizeof(gpu->driver));
    gpu->temperature = 45.0f;
    gpu->memory_mb = 6144;
    gpu->usage_percent = 25.0f;
    gpu->clock_speed = 1785;

    return 0;
}

/**
 * @brief Получение статистики GPU (заглушка)
 */
static inline int get_gpu_stats(gpu_stats_t *stats) {
    if (!stats) return -1;

    stats->memory_used = 2048ULL * 1024 * 1024; /* 2GB */
    stats->memory_total = 6144ULL * 1024 * 1024; /* 6GB */
    stats->utilization_percent = 25.0f;
    stats->temperature = 45;
    stats->fan_speed = 1200;

    return 0;
}

/**
 * @brief Получение средней нагрузки (заглушка)
 */
static inline int get_load_average(float *load_avg, int count) {
    if (!load_avg || count <= 0) return -1;

    for (int i = 0; i < count && i < 3; i++) {
        load_avg[i] = 1.2f + i * 0.3f; /* 1.2, 1.5, 1.8 */
    }

    return 0;
}

/* ============================================================================
 * Заглушки функций инструментов разработки
 * ============================================================================ */

/**
 * @brief Получение списка слушающих портов (заглушка)
 */
static inline int get_listening_ports(port_info_t *ports, int max_count, int *count) {
    if (!ports || !count || max_count <= 0) return -1;

    *count = 0;

    /* Создание фиктивных портов для тестирования */
    for (int i = 0; i < max_count && i < 3; i++) {
        ports[i].port = 8080 + i;
        safe_strcpy(ports[i].protocol, "TCP", sizeof(ports[i].protocol));
        snprintf(ports[i].process_name, sizeof(ports[i].process_name), "test_service_%d", i);
        ports[i].pid = 2000 + i;
        safe_strcpy(ports[i].state, "LISTEN", sizeof(ports[i].state));
        (*count)++;
    }

    return 0;
}

/**
 * @brief Получение окружения разработки (заглушка)
 */
static inline int get_dev_environment(dev_environment_t *env) {
    if (!env) return -1;

    safe_strcpy(env->ide_name, "VSCode", sizeof(env->ide_name));
    safe_strcpy(env->languages, "C,C++,Python,JavaScript", sizeof(env->languages));
    safe_strcpy(env->frameworks, "SystemMonitor,TestFramework", sizeof(env->frameworks));
    safe_strcpy(env->tools, "GCC,Make,Git,Docker", sizeof(env->tools));
    env->debugging_enabled = true;

    return 0;
}

/**
 * @brief Получение процессов разработки (заглушка)
 */
static inline int get_dev_processes(dev_process_t *processes, int max_count, int *count) {
    if (!processes || !count || max_count <= 0) return -1;

    *count = 0;

    /* Создание фиктивных процессов разработки */
    for (int i = 0; i < max_count && i < 2; i++) {
        processes[i].pid = 3000 + i;
        snprintf(processes[i].name, sizeof(processes[i].name), "dev_tool_%d", i);
        if (i == 0) {
            safe_strcpy(processes[i].type, "IDE", sizeof(processes[i].type));
        } else {
            safe_strcpy(processes[i].type, "Compiler", sizeof(processes[i].type));
        }
        processes[i].cpu_usage = 20.0f + i * 10.0f;
        processes[i].mem_usage = 15.0f + i * 5.0f;
        processes[i].memory_bytes = (200 + i * 100) * 1024 * 1024;
        processes[i].start_time = 1234567890 + i * 1800;
        (*count)++;
    }

    return 0;
}

#endif // PLATFORM_FUNCTIONS_STUB_H
>>>>>>> REPLACE
</diff>
</apply_diff>