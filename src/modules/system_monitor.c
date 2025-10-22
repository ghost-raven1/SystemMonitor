/**
 * @file system_monitor.c
 * @brief Реализация модуля мониторинга системных ресурсов
 *
 * Модуль предоставляет унифицированный интерфейс для сбора информации
 * о CPU, памяти, процессах и других системных ресурсах.
 */

#include "modules/system_monitor.h"
#include "platform/system_info.h"
#include "platform/processes.h"
#include "core/process_tree.h"
#include "platform/battery.h"
#include "platform/usb.h"
#include "platform/gpu.h"
#include "core/anomalies.h"
#include "platform/network.h"
#include "platform/disk.h"
#include "platform/smart.h"
#include "platform/mac_smc.h"
#include "utils/logging.h"
#include "utils/config.h"
#include "platform/platform.h"
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <pthread.h>

// Глобальное состояние модуля мониторинга
static system_monitor_state_t g_state = {
    .cpu_monitor_enabled = true,
    .memory_monitor_enabled = true,
    .process_monitor_enabled = true,
    .system_monitor_enabled = true,
    .update_interval_ms = 2000,
    .last_update = 0
};

// Глобальные переменные для хранения истории метрик
static float cpu_history[60];
static float mem_history[60];
static int history_index = 0;
static int cycles = 0;

// Информация о батарее
static battery_info_t battery_info;

// Мьютекс для защиты глобального состояния
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

// Коллбэки для обновления данных
static system_update_callback_t g_callback = NULL;

/**
 * @brief Инициализация модуля мониторинга системы
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_init(void) {
    if (pthread_mutex_init(&state_mutex, NULL) != 0) {
        log_error("Не удалось инициализировать мьютекс модуля мониторинга");
        return -1;
    }

    // Инициализируем историю метрик нулями
    memset(cpu_history, 0, sizeof(cpu_history));
    memset(mem_history, 0, sizeof(mem_history));
    history_index = 0;
    cycles = 0;

    // Инициализируем информацию о батарее
    memset(&battery_info, 0, sizeof(battery_info_t));
    battery_info.percentage = -1;

    // Устанавливаем время последнего обновления
    g_state.last_update = time(NULL);

    log_info("Модуль мониторинга системы инициализирован");
    return 0;
}

/**
 * @brief Деинициализация модуля мониторинга системы
 */
void system_monitor_cleanup(void) {
    pthread_mutex_lock(&state_mutex);

    // Отключаем все мониторы
    g_state.cpu_monitor_enabled = false;
    g_state.memory_monitor_enabled = false;
    g_state.process_monitor_enabled = false;
    g_state.system_monitor_enabled = false;

    // Очищаем коллбэк
    g_callback = NULL;

    pthread_mutex_unlock(&state_mutex);
    pthread_mutex_destroy(&state_mutex);

    log_info("Модуль мониторинга системы деинициализирован");
}

/**
 * @brief Запуск мониторинга системы
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_start(void) {
    pthread_mutex_lock(&state_mutex);

    g_state.cpu_monitor_enabled = true;
    g_state.memory_monitor_enabled = true;
    g_state.process_monitor_enabled = true;
    g_state.system_monitor_enabled = true;

    g_state.last_update = time(NULL);

    pthread_mutex_unlock(&state_mutex);

    log_info("Мониторинг системы запущен");
    return 0;
}

/**
 * @brief Остановка мониторинга системы
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_stop(void) {
    pthread_mutex_lock(&state_mutex);

    g_state.cpu_monitor_enabled = false;
    g_state.memory_monitor_enabled = false;
    g_state.process_monitor_enabled = false;
    g_state.system_monitor_enabled = false;

    pthread_mutex_unlock(&state_mutex);

    log_info("Мониторинг системы остановлен");
    return 0;
}

/**
 * @brief Получение информации о CPU
 * @param info Структура для записи информации о CPU
 * @return 0 при успехе, -1 при ошибке
 */
int get_cpu_info(cpu_info_t *info) {
    if (!info) return -1;

    pthread_mutex_lock(&state_mutex);

    if (!g_state.cpu_monitor_enabled) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем использование CPU
    float usage = get_cpu_usage();
    if (usage < 0) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем количество ядер
    int core_count = 0;
    size_t size = sizeof(core_count);
    if (sysctlbyname("hw.ncpu", &core_count, &size, NULL, 0) != 0) {
        core_count = 1; // Значение по умолчанию
    }

    // Получаем температуру CPU (если доступна)
    float temperature = -1.0f;
    float core_temps[8];
    int temp_count = 0;
    if (smc_get_core_temperatures(core_temps, 8, &temp_count) == 0 && temp_count > 0) {
        temperature = core_temps[0]; // Используем температуру первого ядра
    }

    // Получаем частоту CPU
    long long current_freq = -1, max_freq = -1;
    get_cpu_frequencies(&current_freq, &max_freq);

    float frequency = -1.0f;
    if (current_freq > 0) {
        frequency = (float)current_freq / 1000000.0f; // Конвертируем в MHz
    }

    // Заполняем структуру
    info->usage_percent = usage;
    info->core_count = core_count;
    info->temperature = temperature;
    info->frequency = frequency;

    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Получение информации о памяти
 * @param info Структура для записи информации о памяти
 * @return 0 при успехе, -1 при ошибке
 */
int get_memory_info(memory_info_t *info) {
    if (!info) return -1;

    pthread_mutex_lock(&state_mutex);

    if (!g_state.memory_monitor_enabled) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем использование памяти
    float usage_percent = get_memory_usage();
    if (usage_percent < 0) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем детальную информацию о памяти
    memory_breakdown_t mem_breakdown;
    if (get_memory_breakdown(&mem_breakdown) != 0) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Конвертируем байты в мегабайты
    unsigned long total_mb = (unsigned long)(mem_breakdown.total_bytes / (1024 * 1024));
    unsigned long used_mb = (unsigned long)((mem_breakdown.active_bytes +
                                           mem_breakdown.inactive_bytes +
                                           mem_breakdown.wired_bytes) / (1024 * 1024));
    unsigned long free_mb = (unsigned long)(mem_breakdown.free_bytes / (1024 * 1024));

    // Получаем информацию о swap
    unsigned long swap_total_mb = 0, swap_used_mb = 0;
    size_t size = sizeof(unsigned long);
    if (sysctlbyname("vm.swapusage", NULL, NULL, NULL, 0) == 0) {
        struct xsw_usage swap_info;
        size = sizeof(swap_info);
        if (sysctlbyname("vm.swapusage", &swap_info, &size, NULL, 0) == 0) {
            swap_total_mb = (unsigned long)(swap_info.xsu_total / (1024 * 1024));
            swap_used_mb = (unsigned long)(swap_info.xsu_used / (1024 * 1024));
        }
    }

    // Заполняем структуру
    info->total_mb = total_mb;
    info->used_mb = used_mb;
    info->free_mb = free_mb;
    info->usage_percent = usage_percent;
    info->swap_total_mb = swap_total_mb;
    info->swap_used_mb = swap_used_mb;

    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Получение информации о системе
 * @param info Структура для записи информации о системе
 * @return 0 при успехе, -1 при ошибке
 */
int get_system_monitor_system_info(system_info_t *info) {
    if (!info) return -1;

    pthread_mutex_lock(&state_mutex);

    if (!g_state.system_monitor_enabled) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем имя платформы
    platform_t platform = detect_platform();
    strncpy(info->platform, platform_name(platform), sizeof(info->platform) - 1);
    info->platform[sizeof(info->platform) - 1] = '\0';

    // Получаем имя хоста
    if (gethostname(info->hostname, sizeof(info->hostname)) != 0) {
        strncpy(info->hostname, "unknown", sizeof(info->hostname) - 1);
    }
    info->hostname[sizeof(info->hostname) - 1] = '\0';

    // Получаем версию ОС
    size_t size = sizeof(info->os_version);
    if (sysctlbyname("kern.osversion", info->os_version, &size, NULL, 0) != 0) {
        strncpy(info->os_version, "unknown", sizeof(info->os_version) - 1);
    }
    info->os_version[sizeof(info->os_version) - 1] = '\0';

    // Получаем время работы системы
    info->uptime_seconds = (int)get_uptime_seconds();

    // Получаем время загрузки системы
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) == 0) {
        info->boot_time = boottime.tv_sec;
    } else {
        info->boot_time = time(NULL) - info->uptime_seconds;
    }

    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Получение списка процессов
 * @param processes Массив для записи процессов
 * @param count Максимальное количество процессов для записи
 * @return Количество записанных процессов или -1 при ошибке
 */
int get_system_monitor_process_list(process_info_t **processes, int *count) {
    if (!processes || !count || *count <= 0) return -1;

    pthread_mutex_lock(&state_mutex);

    if (!g_state.process_monitor_enabled) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Выделяем память для процессов
    *processes = malloc(*count * sizeof(process_info_t));
    if (!*processes) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем список процессов через платформенный интерфейс
    size_t actual_count = get_process_list_cached(*processes, *count);

    *count = (int)actual_count;

    pthread_mutex_unlock(&state_mutex);
    return (int)actual_count;
}

/**
 * @brief Получение информации о конкретном процессе
 * @param pid ID процесса
 * @param info Структура для записи информации о процессе
 * @return 0 при успехе, -1 при ошибке
 */
int get_process_info(int pid, process_info_t *info) {
    if (!info || pid <= 0) return -1;

    pthread_mutex_lock(&state_mutex);

    if (!g_state.process_monitor_enabled) {
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    // Получаем информацию о процессе
    process_info_t procs[1];
    size_t count = get_process_list_cached(procs, 1);

    int found = 0;
    for (size_t i = 0; i < count; i++) {
        if (procs[i].pid == pid) {
            memcpy(info, &procs[i], sizeof(process_info_t));
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&state_mutex);

    return found ? 0 : -1;
}

/**
 * @brief Освобождение списка процессов
 * @param processes Список процессов для освобождения
 */
void free_process_list(process_info_t *processes) {
    if (processes) {
        free(processes);
    }
}

/**
 * @brief Регистрация коллбэка для обновления данных
 * @param callback Функция коллбэка
 * @return 0 при успехе, -1 при ошибке
 */
int register_system_update_callback(system_update_callback_t callback) {
    if (!callback) return -1;

    pthread_mutex_lock(&state_mutex);
    g_callback = callback;
    pthread_mutex_unlock(&state_mutex);

    return 0;
}

/**
 * @brief Удаление регистрации коллбэка для обновления данных
 * @param callback Функция коллбэка
 * @return 0 при успехе, -1 при ошибке
 */
int unregister_system_update_callback(system_update_callback_t callback) {
    if (!callback) return -1;

    pthread_mutex_lock(&state_mutex);

    if (g_callback == callback) {
        g_callback = NULL;
    }

    pthread_mutex_unlock(&state_mutex);

    return 0;
}

/**
 * @brief Включение/выключение мониторинга CPU
 * @param enabled Флаг включения
 * @return 0 при успехе, -1 при ошибке
 */
int set_cpu_monitoring(bool enabled) {
    pthread_mutex_lock(&state_mutex);
    g_state.cpu_monitor_enabled = enabled;
    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Включение/выключение мониторинга памяти
 * @param enabled Флаг включения
 * @return 0 при успехе, -1 при ошибке
 */
int set_memory_monitoring(bool enabled) {
    pthread_mutex_lock(&state_mutex);
    g_state.memory_monitor_enabled = enabled;
    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Включение/выключение мониторинга процессов
 * @param enabled Флаг включения
 * @return 0 при успехе, -1 при ошибке
 */
int set_process_monitoring(bool enabled) {
    pthread_mutex_lock(&state_mutex);
    g_state.process_monitor_enabled = enabled;
    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Установка интервала обновления
 * @param interval_ms Интервал в миллисекундах
 * @return 0 при успехе, -1 при ошибке
 */
int set_update_interval(int interval_ms) {
    if (interval_ms < 100 || interval_ms > 60000) return -1; // 100мс - 60с

    pthread_mutex_lock(&state_mutex);
    g_state.update_interval_ms = interval_ms;
    pthread_mutex_unlock(&state_mutex);

    return 0;
}

/**
 * @brief Получение состояния модуля мониторинга
 * @return Указатель на структуру состояния
 */
const system_monitor_state_t *get_system_monitor_state(void) {
    return &g_state;
}

/**
 * @brief Получение использования CPU через интерфейс модуля
 * @return Процент использования CPU или -1 при ошибке
 */
float system_monitor_get_cpu_usage(void) {
    cpu_info_t info;
    if (get_cpu_info(&info) == 0) {
        return info.usage_percent;
    }
    return -1.0f;
}

/**
 * @brief Получение использования памяти через интерфейс модуля
 * @return Процент использования памяти или -1 при ошибке
 */
float system_monitor_get_memory_usage(void) {
    memory_info_t info;
    if (get_memory_info(&info) == 0) {
        return info.usage_percent;
    }
    return -1.0f;
}

/**
 * @brief Обновление истории метрик
 */
void system_monitor_update_history(void) {
    pthread_mutex_lock(&state_mutex);

    // Получаем текущие метрики
    float cpu = get_cpu_usage();
    float mem = get_memory_usage();

    // Проверяем валидность данных
    if (cpu < 0) cpu = 0.0f;
    if (mem < 0) mem = 0.0f;

    // Обновляем историю
    cpu_history[history_index] = cpu;
    mem_history[history_index] = mem;

    // Увеличиваем индекс истории
    history_index = (history_index + 1) % 60;

    // Увеличиваем счетчик циклов
    cycles++;

    // Обновляем время последнего обновления
    g_state.last_update = time(NULL);

    // Вызываем коллбэк если он зарегистрирован
    if (g_callback) {
        cpu_info_t cpu_info;
        memory_info_t mem_info;
        system_info_t sys_info;

        // Получаем детальную информацию для коллбэка
        get_cpu_info(&cpu_info);
        get_memory_info(&mem_info);
        get_system_monitor_system_info(&sys_info);

        g_callback(&cpu_info, &mem_info, &sys_info);
    }

    pthread_mutex_unlock(&state_mutex);
}

/**
 * @brief Получение истории использования CPU
 * @param history Массив для записи истории (должен быть размером 60)
 * @param count Количество элементов для записи
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_cpu_history(float *history, int count) {
    if (!history || count <= 0 || count > 60) return -1;

    pthread_mutex_lock(&state_mutex);

    // Копируем историю CPU
    for (int i = 0; i < count && i < 60; i++) {
        int index = (history_index - count + i + 60) % 60;
        history[i] = cpu_history[index];
    }

    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Получение истории использования памяти
 * @param history Массив для записи истории (должен быть размером 60)
 * @param count Количество элементов для записи
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_memory_history(float *history, int count) {
    if (!history || count <= 0 || count > 60) return -1;

    pthread_mutex_lock(&state_mutex);

    // Копируем историю памяти
    for (int i = 0; i < count && i < 60; i++) {
        int index = (history_index - count + i + 60) % 60;
        history[i] = mem_history[index];
    }

    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Получение количества циклов обновления
 * @return Количество циклов обновления
 */
int system_monitor_get_cycles(void) {
    pthread_mutex_lock(&state_mutex);
    int result = cycles;
    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации о батарее
 * @param info Структура для записи информации о батарее
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_battery_info(battery_info_t *info) {
    if (!info) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем информацию о батарее
    int result = get_battery_info(&battery_info);

    if (result == 0) {
        memcpy(info, &battery_info, sizeof(battery_info_t));
    }

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации о GPU
 * @param info Структура для записи информации о GPU
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_gpu_info(gpu_info_t *info) {
    if (!info) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем информацию о GPU
    int result = get_gpu_info(info);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение температур ядер CPU
 * @param temperatures Массив для записи температур
 * @param max_count Максимальное количество ядер
 * @param written Указатель на количество записанных температур
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_core_temperatures(float *temperatures, int max_count, int *written) {
    if (!temperatures || max_count <= 0) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем температуры ядер CPU
    int result = smc_get_core_temperatures(temperatures, max_count, written);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение температуры памяти
 * @param temperature Указатель на переменную для записи температуры
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_memory_temperature(float *temperature) {
    if (!temperature) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем температуру памяти
    int result = smc_get_mem_temperature(temperature);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации о диске
 * @return Строка с информацией о диске или NULL при ошибке
 */
const char *system_monitor_get_disk_info(void) {
    pthread_mutex_lock(&state_mutex);

    // Получаем информацию о диске
    const char *result = get_disk_info();

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации о сети
 * @return Строка с информацией о сети или NULL при ошибке
 */
const char *system_monitor_get_network_info(void) {
    pthread_mutex_lock(&state_mutex);

    // Получаем информацию о сети
    const char *result = get_network_info();

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации о процессах на ядро
 * @param usage Массив для записи использования CPU каждым ядром
 * @param max_cores Максимальное количество ядер
 * @param written Указатель на количество записанных ядер
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_per_core_usage(float *usage, int max_cores, int *written) {
    if (!usage || max_cores <= 0) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем использование CPU каждым ядром
    int result = get_per_core_usage(usage, max_cores, written);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение частот CPU
 * @param current_hz Указатель на текущую частоту в Hz
 * @param max_hz Указатель на максимальную частоту в Hz
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_cpu_frequencies(long long *current_hz, long long *max_hz) {
    pthread_mutex_lock(&state_mutex);

    // Получаем частоты CPU
    int result = get_cpu_frequencies(current_hz, max_hz);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение времени работы системы в секундах
 * @return Время работы системы в секундах или -1 при ошибке
 */
long system_monitor_get_uptime_seconds(void) {
    pthread_mutex_lock(&state_mutex);

    // Получаем время работы системы
    long result = get_uptime_seconds();

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации об USB устройствах
 * @param devices Массив для записи устройств
 * @param max_count Максимальное количество устройств
 * @param count Указатель на количество записанных устройств
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_usb_devices(usb_device_t *devices, int max_count, int *count) {
    if (!devices || max_count <= 0) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем информацию об USB устройствах
    int result = list_usb_devices(devices, max_count, count);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Получение информации о SMART дисках
 * @param device_name Имя устройства (может быть NULL)
 * @param info Структура для записи информации о SMART
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_smart_info(const char *device_name, smart_info_t *info) {
    if (!info) return -1;

    pthread_mutex_lock(&state_mutex);

    // Получаем информацию о SMART
    int result = get_smart_info(device_name, info);

    pthread_mutex_unlock(&state_mutex);
    return result;
}

/**
 * @brief Проверка доступности модулей мониторинга
 * @return Битовая маска доступных модулей
 */
int system_monitor_check_modules(void) {
    int available = 0;

    pthread_mutex_lock(&state_mutex);

    // Проверяем доступность различных модулей
    if (g_state.cpu_monitor_enabled) available |= 1;
    if (g_state.memory_monitor_enabled) available |= 2;
    if (g_state.process_monitor_enabled) available |= 4;
    if (g_state.system_monitor_enabled) available |= 8;

    // Проверяем фактическую доступность функций
    float cpu = get_cpu_usage();
    if (cpu >= 0) available |= 16;

    float mem = get_memory_usage();
    if (mem >= 0) available |= 32;

    battery_info_t bat;
    if (get_battery_info(&bat) == 0) available |= 64;

    gpu_info_t gpu;
    if (get_gpu_info(&gpu) == 0) available |= 128;

    pthread_mutex_unlock(&state_mutex);

    return available;
}

/**
 * @brief Сброс состояния модуля мониторинга
 */
void system_monitor_reset(void) {
    pthread_mutex_lock(&state_mutex);

    // Сбрасываем историю метрик
    memset(cpu_history, 0, sizeof(cpu_history));
    memset(mem_history, 0, sizeof(mem_history));
    history_index = 0;
    cycles = 0;

    // Сбрасываем информацию о батарее
    memset(&battery_info, 0, sizeof(battery_info_t));
    battery_info.percentage = -1;

    // Обновляем время последнего обновления
    g_state.last_update = time(NULL);

    pthread_mutex_unlock(&state_mutex);

    log_info("Состояние модуля мониторинга сброшено");
}

/**
 * @brief Получение детальной статистики модуля мониторинга
 * @param stats Структура для записи статистики
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_get_stats(system_monitor_stats_t *stats) {
    if (!stats) return -1;

    pthread_mutex_lock(&state_mutex);

    stats->cycles = cycles;
    stats->history_index = history_index;
    stats->last_update = g_state.last_update;
    stats->update_interval_ms = g_state.update_interval_ms;

    // Подсчитываем средние значения истории
    float cpu_sum = 0, mem_sum = 0;
    int valid_samples = 0;

    for (int i = 0; i < 60; i++) {
        if (cpu_history[i] >= 0 && mem_history[i] >= 0) {
            cpu_sum += cpu_history[i];
            mem_sum += mem_history[i];
            valid_samples++;
        }
    }

    if (valid_samples > 0) {
        stats->avg_cpu_usage = cpu_sum / valid_samples;
        stats->avg_memory_usage = mem_sum / valid_samples;
    } else {
        stats->avg_cpu_usage = 0.0f;
        stats->avg_memory_usage = 0.0f;
    }

    stats->valid_samples = valid_samples;

    pthread_mutex_unlock(&state_mutex);
    return 0;
}

/**
 * @brief Обновление метрик системы (основная функция для главного цикла)
 * @return 0 при успехе, -1 при ошибке
 */
int system_monitor_update_metrics(void) {
    if (!g_state.cpu_monitor_enabled && !g_state.memory_monitor_enabled &&
        !g_state.process_monitor_enabled && !g_state.system_monitor_enabled) {
        return -1;
    }

    // Обновляем историю метрик и вызываем коллбэки
    system_monitor_update_history();

    return 0;
}

/**
 * @brief Получение информации о системном мониторе (обертка для UI)
 * @param info Структура для записи информации о системе
 * @return 0 при успехе, -1 при ошибке
 */
int get_system_monitor_info(system_info_t *info) {
    return get_system_monitor_system_info(info);
}

/**
 * @brief Получение списка системных процессов (обертка для UI)
 * @param processes Массив для записи процессов
 * @param count Количество процессов
 * @return Количество записанных процессов или -1 при ошибке
 */
int get_system_process_list(system_process_info_t **processes, int *count) {
    if (!processes || !count || *count <= 0) return -1;

    // Выделяем память для процессов
    *processes = malloc(*count * sizeof(system_process_info_t));
    if (!*processes) {
        return -1;
    }

    // Получаем список процессов через платформенный интерфейс
    process_info_t *platform_processes = NULL;
    size_t actual_count = get_process_list_cached(platform_processes, *count);

    if (actual_count > 0 && platform_processes) {
        // Конвертируем формат процессов
        for (size_t i = 0; i < actual_count && i < (size_t)*count; i++) {
            (*processes)[i].pid = platform_processes[i].pid;
            (*processes)[i].ppid = 0; // Неизвестен в платформенной структуре
            strncpy((*processes)[i].name, platform_processes[i].name, sizeof((*processes)[i].name) - 1);
            (*processes)[i].name[sizeof((*processes)[i].name) - 1] = '\0';
            strncpy((*processes)[i].user, "unknown", sizeof((*processes)[i].user) - 1); // Неизвестен в платформенной структуре
            (*processes)[i].user[sizeof((*processes)[i].user) - 1] = '\0';
            (*processes)[i].cpu_percent = platform_processes[i].cpu_usage;
            (*processes)[i].memory_kb = (unsigned long)(platform_processes[i].mem_usage * 1024); // Конвертируем из MB в KB
            (*processes)[i].thread_count = 1; // Неизвестен в платформенной структуре
            (*processes)[i].start_time = time(NULL); // Неизвестен в платформенной структуре
        }

        free(platform_processes);
        *count = (int)actual_count;
        return (int)actual_count;
    }

    // Освобождаем память при ошибке
    free(*processes);
    *processes = NULL;
    return -1;
}

/**
 * @brief Освобождение списка системных процессов (обертка для UI)
 * @param processes Список процессов для освобождения
 */
void free_system_process_list(system_process_info_t *processes) {
    if (processes) {
        free(processes);
    }
}