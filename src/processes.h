#ifndef PROCESSES_H
#define PROCESSES_H

#include <stddef.h>

typedef struct {
    int pid;
    char name[256];
    float cpu_usage;
    float mem_usage;
} process_info_t;

size_t get_process_list(process_info_t *list, size_t max);

#endif
