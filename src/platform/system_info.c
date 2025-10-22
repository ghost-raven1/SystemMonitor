#include "platform/system_info.h"
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

float get_cpu_usage() {
    static host_cpu_load_info_data_t prev = {0};
    static int initialized = 0;

    natural_t cpuCount = 0;
    processor_info_array_t cpuInfo = NULL;
    mach_msg_type_number_t numCPUInfo = 0;

    kern_return_t kr = host_processor_info(mach_host_self(),
                                           PROCESSOR_CPU_LOAD_INFO,
                                           &cpuCount,
                                           &cpuInfo,
                                           &numCPUInfo);
    if (kr != KERN_SUCCESS || !cpuInfo || cpuCount == 0) return -1;

    unsigned long long user = 0, system = 0, idle = 0;
    for (unsigned i = 0; i < cpuCount; i++) {
        user   += cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_USER];
        system += cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_SYSTEM];
        idle   += cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_IDLE];
    }
    if (cpuInfo) {
        vm_deallocate(mach_task_self(), (vm_address_t)cpuInfo, numCPUInfo * sizeof(integer_t));
    }

    unsigned long long diffUser, diffSystem, diffIdle, diffTotal;
    float usage = 0.0;

    if (initialized) {
        diffUser   = user - prev.cpu_ticks[CPU_STATE_USER];
        diffSystem = system - prev.cpu_ticks[CPU_STATE_SYSTEM];
        diffIdle   = idle - prev.cpu_ticks[CPU_STATE_IDLE];
        diffTotal  = diffUser + diffSystem + diffIdle;
        if (diffTotal > 0) {
            usage = (float)(diffUser + diffSystem) / diffTotal * 100.0f;
        }
    }

    prev.cpu_ticks[CPU_STATE_USER] = user;
    prev.cpu_ticks[CPU_STATE_SYSTEM] = system;
    prev.cpu_ticks[CPU_STATE_IDLE] = idle;
    initialized = 1;

    return usage;
}

// Кеширование для производительности
static long cached_page_size = 0;
static time_t page_size_cache_time = 0;

float get_memory_usage() {
    static time_t cache_time = 0;
    static float cached_result = -1.0f;

    time_t now = time(NULL);

    // Кешируем результат на 1 секунду для производительности
    if (cache_time > 0 && (now - cache_time) < 1 && cached_result >= 0) {
        return cached_result;
    }

    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if (host_statistics(mach_host_self(), HOST_VM_INFO,
                         (host_info_t)&vmstat, &count) != KERN_SUCCESS) {
        return -1;
    }

    // Кешируем page_size на 5 минут
    if (cached_page_size == 0 || (now - page_size_cache_time) > 300) {
        cached_page_size = sysconf(_SC_PAGESIZE);
        page_size_cache_time = now;
    }

    long page_size = cached_page_size;
    if (page_size <= 0) return -1.0f;

    int64_t free = vmstat.free_count * page_size;
    int64_t active = vmstat.active_count * page_size;
    int64_t inactive = vmstat.inactive_count * page_size;
    int64_t wired = vmstat.wire_count * page_size;
    int64_t total = free + active + inactive + wired;

    if (total <= 0) return -1.0f;

    cached_result = (float)(active + inactive + wired) / total * 100.0f;
    cache_time = now;

    return cached_result;
}

int get_memory_breakdown(memory_breakdown_t *out) {
    if (!out) return -1;
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if (host_statistics(mach_host_self(), HOST_VM_INFO,
                        (host_info_t)&vmstat, &count) != KERN_SUCCESS) {
        return -1;
    }
    long page = sysconf(_SC_PAGESIZE);
    out->free_bytes = (unsigned long long)vmstat.free_count * (unsigned long long)page;
    out->active_bytes = (unsigned long long)vmstat.active_count * (unsigned long long)page;
    out->inactive_bytes = (unsigned long long)vmstat.inactive_count * (unsigned long long)page;
    out->wired_bytes = (unsigned long long)vmstat.wire_count * (unsigned long long)page;
    out->total_bytes = out->free_bytes + out->active_bytes + out->inactive_bytes + out->wired_bytes;
    return 0;
}

int get_cpu_frequencies(long long *current_hz, long long *max_hz) {
    if (current_hz) *current_hz = -1;
    if (max_hz) *max_hz = -1;
    // macOS exposes hw.cpufrequency and hw.cpufrequency_max (in Hz)
    long long cur = -1, maxv = -1;
    size_t sz = sizeof(long long);
    if (sysctlbyname("hw.cpufrequency", &cur, &sz, NULL, 0) == 0 && sz == sizeof(long long)) {
        if (current_hz) *current_hz = cur;
    }
    sz = sizeof(long long);
    if (sysctlbyname("hw.cpufrequency_max", &maxv, &sz, NULL, 0) == 0 && sz == sizeof(long long)) {
        if (max_hz) *max_hz = maxv;
    }
    return 0;
}

long get_uptime_seconds() {
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};

    if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0) {
        return -1;
    }

    time_t bsec = boottime.tv_sec;
    time_t csec = time(NULL);

    return (long) difftime(csec, bsec);
}

int get_system_info(void *info) {
    // Заглушка для совместимости с новым интерфейсом
    // В будущем здесь будет реализация для заполнения структуры system_info_t
    if (info) {
        // Здесь можно было бы заполнить структуру, но для совместимости просто возвращаем 0
        return 0;
    }
    return -1;
}

int get_per_core_usage(float *out, int max_cores, int *written) {
    if (!out || max_cores <= 0) return -1;
    processor_info_array_t cpuInfo = NULL;
    mach_msg_type_number_t numCPUInfo = 0;
    natural_t cpuCount = 0;
    kern_return_t kr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, &cpuInfo, &numCPUInfo);
    if (kr != KERN_SUCCESS || !cpuInfo || cpuCount == 0) return -1;

    static unsigned long long prev_user[256] = {0};
    static unsigned long long prev_system[256] = {0};
    static unsigned long long prev_idle[256] = {0};
    static int initialized = 0;

    int n = (int)cpuCount;
    if (n > max_cores) n = max_cores;
    if (n > 256) n = 256; // Защита от переполнения массива
    for (int i = 0; i < n && i < 256; i++) {
        unsigned long long user = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_USER];
        unsigned long long system = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_SYSTEM];
        unsigned long long idle = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_IDLE];
        float usage = 0.0f;
        if (initialized) {
            unsigned long long du = user - prev_user[i];
            unsigned long long ds = system - prev_system[i];
            unsigned long long di = idle - prev_idle[i];
            unsigned long long dt = du + ds + di;
            if (dt > 0) usage = (float)(du + ds) / (float)dt * 100.0f;
        }
        out[i] = usage;
        prev_user[i] = user;
        prev_system[i] = system;
        prev_idle[i] = idle;
    }
    initialized = 1;
    if (cpuInfo) {
        vm_deallocate(mach_task_self(), (vm_address_t)cpuInfo, numCPUInfo * sizeof(integer_t));
    }
    if (written) *written = n;
    return 0;
}
