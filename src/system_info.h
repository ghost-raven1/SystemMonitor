#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

float get_cpu_usage();
float get_memory_usage();
long get_uptime_seconds();
const char *get_system_info();
int get_per_core_usage(float *out, int max_cores, int *written);

typedef struct {
    unsigned long long free_bytes;
    unsigned long long active_bytes;
    unsigned long long inactive_bytes;
    unsigned long long wired_bytes;
    unsigned long long total_bytes;
} memory_breakdown_t;

int get_memory_breakdown(memory_breakdown_t *out);

// Returns current and max CPU frequency in Hz; values <0 if unknown
int get_cpu_frequencies(long long *current_hz, long long *max_hz);

#endif
