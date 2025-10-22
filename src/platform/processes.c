#include "platform/processes.h"
#include <libproc.h>
#include <sys/sysctl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

// Кеш для оптимизации производительности
static struct {
    process_info_t *processes;
    size_t count;
    size_t capacity;
    time_t last_update;
    pthread_mutex_t mutex;
    int initialized;
} process_cache = {0};

int get_process_list(process_info_t **processes, int *count) {
    if (!processes || !count || *count <= 0) return -1;

    int pids[2048];
    int pid_count = proc_listallpids(pids, sizeof(pids));
    if (pid_count <= 0) return 0;

    // Выделяем память для процессов
    *processes = malloc(*count * sizeof(process_info_t));
    if (!*processes) return -1;

    size_t n = 0;
    for (int i = 0; i < pid_count && n < (size_t)*count && i < 2048; i++) {
        int pid = pids[i];
        if (pid <= 0) continue;

        struct proc_bsdinfo info;
        if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) <= 0)
            continue;

        process_info_t *p = &(*processes)[n++];
        p->pid = pid;
        strncpy(p->name, info.pbi_name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';

        // CPU% based on delta of total user+system time between calls
        static struct {
            int pid;
            unsigned long long total_time_ns; // user+system
        } cache[4096];
        static size_t cache_len = 0;
        static struct timespec prev_ts = {0, 0};
        static int num_cpus = 0;

        if (num_cpus == 0) {
            int mibcpu[2] = {CTL_HW, HW_NCPU};
            size_t l = sizeof(num_cpus);
            sysctl(mibcpu, 2, &num_cpus, &l, NULL, 0);
            if (num_cpus <= 0) num_cpus = 1;
        }

        struct proc_taskinfo tinfo_cpu;
        float cpu_pct = 0.0f;
        if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &tinfo_cpu, sizeof(tinfo_cpu)) > 0) {
            unsigned long long total_ns = (unsigned long long)tinfo_cpu.pti_total_user + (unsigned long long)tinfo_cpu.pti_total_system;

            // find in cache
            size_t idx = cache_len;
            if (cache_len > 0) {
                for (size_t ci = 0; ci < cache_len && ci < 4096; ci++) {
                    if (cache[ci].pid == pid) { idx = ci; break; }
                }
            }

            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double dt = 0.0;
            if (prev_ts.tv_sec != 0 || prev_ts.tv_nsec != 0) {
                dt = (double)(now.tv_sec - prev_ts.tv_sec) + (double)(now.tv_nsec - prev_ts.tv_nsec)/1e9;
            }

            if (idx == cache_len && cache_len < 4096 && cache_len < (sizeof(cache)/sizeof(cache[0]))) {
                cache[cache_len].pid = pid;
                cache[cache_len].total_time_ns = total_ns;
                cache_len++;
            } else if (idx < cache_len && idx < 4096) {
                unsigned long long prev_ns = cache[idx].total_time_ns;
                unsigned long long delta_ns = (total_ns > prev_ns) ? (total_ns - prev_ns) : 0ULL;
                if (dt > 0.0 && num_cpus > 0) {
                    double cpu_frac = ((double)delta_ns / 1e9) / dt; // fraction of one CPU
                    cpu_pct = (float)(cpu_frac * 100.0 * (1.0));
                    // Normalize by number of CPUs to get 0-100% total across all cores
                    cpu_pct /= (float)num_cpus;
                }
                cache[idx].total_time_ns = total_ns;
            }

            prev_ts = now;
        }
        p->cpu_usage = cpu_pct;

        // MEM% по RSS
        struct proc_taskinfo tinfo;
        if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &tinfo, sizeof(tinfo)) > 0) {
            long rss = tinfo.pti_resident_size;
            long total_mem;
            int mib[2] = {CTL_HW, HW_MEMSIZE};
            size_t len = sizeof(total_mem);
            sysctl(mib, 2, &total_mem, &len, NULL, 0);

            p->mem_usage = (float)rss / total_mem * 100.0f;
        } else {
            p->mem_usage = 0.0f;
        }
    }

    *count = (int)n;
    return (int)n;
}

// Инициализация кеша процессов
int init_process_cache(void) {
    if (process_cache.initialized) return 0;

    process_cache.capacity = 512;
    process_cache.processes = malloc(sizeof(process_info_t) * process_cache.capacity);
    if (!process_cache.processes) return -1;

    pthread_mutex_init(&process_cache.mutex, NULL);
    process_cache.initialized = 1;
    process_cache.count = 0;
    process_cache.last_update = 0;

    return 0;
}

// Очистка кеша процессов
void cleanup_process_cache(void) {
    if (!process_cache.initialized) return;

    pthread_mutex_lock(&process_cache.mutex);
    if (process_cache.processes) {
        free(process_cache.processes);
        process_cache.processes = NULL;
    }
    process_cache.count = 0;
    process_cache.capacity = 0;
    process_cache.initialized = 0;
    pthread_mutex_unlock(&process_cache.mutex);
    pthread_mutex_destroy(&process_cache.mutex);
}

// Оптимизированная функция с кешированием и инкрементальным обновлением
size_t get_process_list_cached(process_info_t *list, size_t max) {
    static const time_t CACHE_TTL = 2; // Кешируем на 2 секунды
    time_t now = time(NULL);

    pthread_mutex_lock(&process_cache.mutex);

    // Проверяем, нужно ли обновить кеш
    if (!process_cache.initialized || process_cache.last_update == 0 ||
        (now - process_cache.last_update) >= CACHE_TTL) {

        // Обновляем кеш с инкрементальным подходом
        update_process_cache_incremental();
    }

    // Копируем данные из кеша
    size_t copy_count = process_cache.count < max ? process_cache.count : max;
    if (copy_count > 0 && list) {
        memcpy(list, process_cache.processes, sizeof(process_info_t) * copy_count);
    }

    pthread_mutex_unlock(&process_cache.mutex);
    return copy_count;
}

// Инкрементальное обновление кеша процессов для лучшей производительности
void update_process_cache_incremental(void) {
    static int pids[2048];
    static size_t last_pid_count = 0;

    // Получаем текущий список PID'ов
    int current_pid_count = proc_listallpids(pids, sizeof(pids));
    if (current_pid_count <= 0) return;

    // Если список PID'ов не изменился, просто обновляем статистику существующих процессов
    if (current_pid_count == last_pid_count && process_cache.count > 0) {
        update_existing_processes_stats();
    } else {
        // Полное обновление кеша
        int count = (int)process_cache.capacity;
        get_process_list(&process_cache.processes, &count);
        process_cache.count = (size_t)count;
        last_pid_count = current_pid_count;
    }

    process_cache.last_update = time(NULL);
}

// Обновление статистики только для существующих процессов (более эффективно)
void update_existing_processes_stats(void) {
    for (size_t i = 0; i < process_cache.count; i++) {
        int pid = process_cache.processes[i].pid;
        if (pid <= 0) continue;

        // Обновляем только CPU и память для существующих процессов
        struct proc_taskinfo tinfo;
        if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &tinfo, sizeof(tinfo)) > 0) {
            // Обновляем время CPU
            unsigned long long total_ns = (unsigned long long)tinfo.pti_total_user + (unsigned long long)tinfo.pti_total_system;

            // Простая аппроксимация CPU usage (можно улучшить)
            process_cache.processes[i].cpu_usage = calculate_cpu_from_ns(total_ns, pid);

            // Обновляем память
            long total_mem;
            int mib[2] = {CTL_HW, HW_MEMSIZE};
            size_t len = sizeof(total_mem);
            sysctl(mib, 2, &total_mem, &len, NULL, 0);
            process_cache.processes[i].mem_usage = (float)tinfo.pti_resident_size / total_mem * 100.0f;
        }
    }
}

// Простая функция для расчета CPU usage из наносекунд
float calculate_cpu_from_ns(unsigned long long total_ns, int pid) {
    static struct {
        int pid;
        unsigned long long prev_ns;
        struct timespec prev_time;
    } cpu_history[512];

    static int history_initialized = 0;
    if (!history_initialized) {
        memset(cpu_history, 0, sizeof(cpu_history));
        history_initialized = 1;
    }

    // Находим или создаем запись для этого PID
    int slot = -1;
    for (int i = 0; i < 512; i++) {
        if (cpu_history[i].pid == pid) {
            slot = i;
            break;
        }
        if (cpu_history[i].pid == 0 && slot == -1) {
            slot = i;
        }
    }

    if (slot == -1) return 0.0f; // Нет места в истории

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (cpu_history[slot].pid == 0) {
        // Первая запись для этого процесса
        cpu_history[slot].pid = pid;
        cpu_history[slot].prev_ns = total_ns;
        cpu_history[slot].prev_time = now;
        return 0.0f;
    }

    // Вычисляем CPU usage
    double dt = (now.tv_sec - cpu_history[slot].prev_time.tv_sec) +
                (now.tv_nsec - cpu_history[slot].prev_time.tv_nsec) / 1e9;

    if (dt > 0.0) {
        unsigned long long delta_ns = total_ns - cpu_history[slot].prev_ns;
        float cpu_usage = (float)((double)delta_ns / 1e9 / dt) * 100.0f;

        // Обновляем историю
        cpu_history[slot].prev_ns = total_ns;
        cpu_history[slot].prev_time = now;

        return cpu_usage > 100.0f ? 100.0f : cpu_usage;
    }

    return 0.0f;
}
