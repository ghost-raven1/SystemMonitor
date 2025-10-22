/**
 * process_tree.h - Расширенный анализ дерева процессов и зависимостей
 *
 * Функционал:
 * - Построение дерева процессов с родительскими связями
 * - Анализ зависимостей между процессами
 * - Визуализация дерева в терминале
 * - Мониторинг ресурсов по дереву процессов
 * - Поиск и фильтрация процессов
 * - Групповые операции с процессами
 */

#ifndef PROCESS_TREE_H
#define PROCESS_TREE_H

#include <sys/types.h>
#include <stdbool.h>

// Структура для представления узла дерева процессов
typedef struct process_tree_node {
    pid_t pid;
    pid_t ppid;           // Parent PID
    char name[256];       // Process name
    char command[512];    // Full command line
    char state[16];       // Process state (R, S, D, T, Z, X)

    // Resource usage
    float cpu_percent;
    float memory_percent;
    unsigned long rss;    // Resident Set Size in KB
    unsigned long vsz;    // Virtual Memory Size in KB

    // Timing information
    unsigned long long utime;    // User time in clock ticks
    unsigned long long stime;    // System time in clock ticks
    unsigned long start_time;    // Start time (seconds since epoch)

    // Tree structure
    struct process_tree_node* parent;
    struct process_tree_node** children;
    int children_count;
    int children_capacity;
    int depth;            // Depth in the tree
    int tree_size;        // Total number of descendants

    // Statistics for the entire subtree
    float subtree_cpu;
    float subtree_memory;
    int thread_count;

    // Metadata
    bool is_kernel_thread;
    bool has_children;
    char user[64];        // Owner of the process
} process_tree_node_t;

// Структура для всего дерева процессов
typedef struct {
    process_tree_node_t* root;
    process_tree_node_t** nodes;  // Array of all nodes for quick access
    int node_count;
    int node_capacity;
    unsigned long long timestamp; // When the tree was built
} process_tree_t;

// Фильтры для поиска процессов
typedef struct {
    pid_t pid;              // Specific PID (-1 for any)
    pid_t ppid;             // Specific parent PID (-1 for any)
    char name_pattern[128]; // Pattern to match process name
    char user[64];          // Owner username
    float min_cpu;          // Minimum CPU usage
    float min_memory;       // Minimum memory usage
    bool kernel_threads;    // Include kernel threads
    bool user_processes;    // Include user processes
    int max_depth;          // Maximum tree depth to traverse
} process_filter_t;

// Статистика для группы процессов
typedef struct {
    int total_processes;
    int total_threads;
    float total_cpu;
    float total_memory;
    float avg_cpu;
    float avg_memory;
    pid_t oldest_pid;
    pid_t newest_pid;
    char most_cpu_process[256];
    char most_memory_process[256];
} process_group_stats_t;

// Функции инициализации и очистки
int process_tree_init(process_tree_t* tree);
void process_tree_destroy(process_tree_t* tree);

// Построение дерева процессов
int process_tree_build(process_tree_t* tree);
int process_tree_refresh(process_tree_t* tree);

// Поиск процессов
process_tree_node_t* process_tree_find_by_pid(process_tree_t* tree, pid_t pid);
process_tree_node_t** process_tree_find_by_name(process_tree_t* tree, const char* pattern, int* count);
process_tree_node_t** process_tree_find_by_cpu(process_tree_t* tree, float min_cpu, int* count);
process_tree_node_t** process_tree_find_by_memory(process_tree_t* tree, float min_memory, int* count);

// Фильтрация процессов
process_tree_node_t** process_tree_filter(process_tree_t* tree, process_filter_t* filter, int* count);

// Работа с деревом
int process_tree_add_child(process_tree_node_t* parent, process_tree_node_t* child);
void process_tree_calculate_subtree_stats(process_tree_node_t* node);
void process_tree_sort_by_cpu(process_tree_node_t** nodes, int count);
void process_tree_sort_by_memory(process_tree_node_t** nodes, int count);

// Статистика
void process_tree_get_stats(process_tree_t* tree, process_group_stats_t* stats);
void process_tree_get_node_stats(process_tree_node_t* node, process_group_stats_t* stats);

// Управление процессами
int process_tree_kill_subtree(process_tree_node_t* root, int signal);
int process_tree_suspend_subtree(process_tree_node_t* root);
int process_tree_resume_subtree(process_tree_node_t* root);

// Экспорт дерева
int process_tree_export_dot(process_tree_t* tree, const char* filename);
int process_tree_export_json(process_tree_t* tree, const char* filename);
int process_tree_export_csv(process_tree_t* tree, const char* filename);
int process_tree_export_txt(process_tree_t* tree, const char* filename);

// Визуализация в терминале
void process_tree_draw_ascii(process_tree_t* tree, int max_depth);
void process_tree_draw_interactive(process_tree_t* tree, int start_y, int height);

// Утилиты
const char* process_state_to_string(char state);
void format_process_info(process_tree_node_t* node, char* buffer, int buffer_size);
int get_process_threads(pid_t pid);

#endif // PROCESS_TREE_H