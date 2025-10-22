#ifndef PROCESSES_H
#define PROCESSES_H

#include <stddef.h>

typedef struct {
    int pid;
    char name[256];
    float cpu_usage;
    float mem_usage;
} process_info_t;

int get_process_list(process_info_t **processes, int *count); // Объявление изменено для совместимости

// Оптимизированные функции для производительности
size_t get_process_list_cached(process_info_t *list, size_t max);
int init_process_cache(void);
void cleanup_process_cache(void);
void update_process_cache_incremental(void);
void update_existing_processes_stats(void);
float calculate_cpu_from_ns(unsigned long long total_ns, int pid);

#endif
