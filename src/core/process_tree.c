/**
 * process_tree.c - Реализация расширенного анализа дерева процессов
 */

#include "core/process_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>

// Путь к procfs
#define PROC_PATH "/proc"

// Структура для хранения информации о процессе из /proc/[pid]/stat
typedef struct {
    pid_t pid;
    char comm[256];      // Имя процесса
    char state;          // Состояние
    pid_t ppid;          // Родительский PID
    unsigned long utime; // Пользовательское время
    unsigned long stime; // Системное время
    long rss;           // RSS в страницах
    unsigned long vsz;   // VSZ в байтах
    long priority;      // Приоритет
    long nice;          // Nice значение
    unsigned long start_time; // Время запуска
} proc_stat_t;

// Структура для хранения информации о памяти из /proc/[pid]/status
typedef struct {
    unsigned long vm_rss;    // RSS в kB
    unsigned long vm_size;   // VSZ в kB
    unsigned long vm_data;   // Data size
    unsigned long vm_stk;    // Stack size
    unsigned long threads;   // Количество потоков
} proc_status_t;

// Чтение информации из /proc/[pid]/stat
static int read_proc_stat(pid_t pid, proc_stat_t* stat) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%d/stat", PROC_PATH, pid);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    // Формат: pid (comm) state ppid ...
    if (fscanf(fp, "%d (%[^)]) %c %d %*d %*d %*d %*d %*d %*d %*d %*d %*d %lu %lu %*d %*d %*d %*d %*d %*d %lu",
               &stat->pid, stat->comm, &stat->state, &stat->ppid,
               &stat->utime, &stat->stime, &stat->start_time) != 7) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

// Чтение информации из /proc/[pid]/status
static int read_proc_status(pid_t pid, proc_status_t* status) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%d/status", PROC_PATH, pid);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu", &status->vm_rss);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line + 7, "%lu", &status->vm_size);
        } else if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line + 8, "%lu", &status->threads);
        }
    }

    fclose(fp);
    return 0;
}

// Получение имени владельца процесса
static int get_process_owner(pid_t pid, char* user, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%d", PROC_PATH, pid);

    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }

    struct passwd* pw = getpwuid(st.st_uid);
    if (pw) {
        strncpy(user, pw->pw_name, size - 1);
        user[size - 1] = '\0';
        return 0;
    }

    snprintf(user, size, "%d", st.st_uid);
    return 0;
}

// Чтение командной строки процесса
static int get_process_cmdline(pid_t pid, char* cmdline, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%d/cmdline", PROC_PATH, pid);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    size_t n = fread(cmdline, 1, size - 1, fp);
    fclose(fp);

    // Заменяем null-байты пробелами
    for (size_t i = 0; i < n; i++) {
        if (cmdline[i] == '\0') {
            cmdline[i] = ' ';
        }
    }
    cmdline[n] = '\0';

    // Удаляем завершающий пробел
    if (n > 0 && cmdline[n-1] == ' ') {
        cmdline[n-1] = '\0';
    }

    return 0;
}

// Создание нового узла дерева
static process_tree_node_t* create_node(pid_t pid) {
    process_tree_node_t* node = calloc(1, sizeof(process_tree_node_t));
    if (!node) {
        return NULL;
    }

    node->pid = pid;
    node->children = NULL;
    node->children_count = 0;
    node->children_capacity = 0;
    node->parent = NULL;
    node->depth = 0;
    node->tree_size = 1;

    return node;
}

// Добавление дочернего узла
static int add_child_node(process_tree_node_t* parent, process_tree_node_t* child) {
    if (parent->children_count >= parent->children_capacity) {
        int new_capacity = parent->children_capacity == 0 ? 4 : parent->children_capacity * 2;
        process_tree_node_t** new_children = realloc(parent->children,
                                                     new_capacity * sizeof(process_tree_node_t*));
        if (!new_children) {
            return -1;
        }
        parent->children = new_children;
        parent->children_capacity = new_capacity;
    }

    parent->children[parent->children_count++] = child;
    child->parent = parent;
    child->depth = parent->depth + 1;

    return 0;
}

// Сбор информации о процессе
static int collect_process_info(process_tree_node_t* node) {
    proc_stat_t stat;
    proc_status_t status;

    // Читаем базовую информацию
    if (read_proc_stat(node->pid, &stat) != 0) {
        return -1;
    }

    node->ppid = stat.ppid;
    node->state[0] = stat.state;
    node->state[1] = '\0';
    strncpy(node->name, stat.comm, sizeof(node->name) - 1);
    node->utime = stat.utime;
    node->stime = stat.stime;
    node->start_time = stat.start_time;

    // Читаем информацию о памяти
    if (read_proc_status(node->pid, &status) == 0) {
        node->rss = status.vm_rss;
        node->vsz = status.vm_size;
        node->thread_count = status.threads;
    }

    // Получаем командную строку
    get_process_cmdline(node->pid, node->command, sizeof(node->command));

    // Получаем владельца
    get_process_owner(node->pid, node->user, sizeof(node->user));

    // Определяем, является ли процесс kernel thread
    node->is_kernel_thread = (node->ppid == 2 || strstr(node->name, "kworker") ||
                             strstr(node->name, "migration") || strstr(node->name, "ksoftirqd"));

    return 0;
}

// Рекурсивное построение дерева
static process_tree_node_t* build_process_subtree(pid_t pid, int depth, int max_depth) {
    if (max_depth > 0 && depth > max_depth) {
        return NULL;
    }

    process_tree_node_t* node = create_node(pid);
    if (!node) {
        return NULL;
    }

    if (collect_process_info(node) != 0) {
        free(node);
        return NULL;
    }

    // Находим дочерние процессы
    DIR* proc_dir = opendir(PROC_PATH);
    if (!proc_dir) {
        free(node);
        return NULL;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;

        pid_t child_pid = atoi(entry->d_name);
        if (child_pid <= 0) continue;

        proc_stat_t child_stat;
        if (read_proc_stat(child_pid, &child_stat) != 0) continue;

        if (child_stat.ppid == pid) {
            process_tree_node_t* child = build_process_subtree(child_pid, depth + 1, max_depth);
            if (child) {
                add_child_node(node, child);
            }
        }
    }

    closedir(proc_dir);
    return node;
}

// Функции инициализации и очистки

int process_tree_init(process_tree_t* tree) {
    if (!tree) return -1;

    memset(tree, 0, sizeof(process_tree_t));
    tree->nodes = NULL;
    tree->node_count = 0;
    tree->node_capacity = 0;

    return 0;
}

void process_tree_destroy(process_tree_t* tree) {
    if (!tree) return;

    // Рекурсивно освобождаем память узлов
    if (tree->root) {
        // Здесь должна быть функция освобождения дерева
        // Пока просто очищаем указатели
    }

    if (tree->nodes) {
        free(tree->nodes);
    }

    memset(tree, 0, sizeof(process_tree_t));
}

// Построение дерева процессов
int process_tree_build(process_tree_t* tree) {
    if (!tree) return -1;

    // Очищаем предыдущее дерево
    process_tree_destroy(tree);

    // Начинаем с процесса init (PID 1)
    tree->root = build_process_subtree(1, 0, 20); // Максимальная глубина 20

    if (!tree->root) {
        return -1;
    }

    // Подсчитываем статистику
    process_tree_calculate_subtree_stats(tree->root);
    tree->timestamp = time(NULL);

    return 0;
}

// Поиск процессов

process_tree_node_t* process_tree_find_by_pid(process_tree_t* tree, pid_t pid) {
    if (!tree || !tree->root) return NULL;

    // Простой линейный поиск (можно улучшить с помощью хэш-таблицы)
    // Пока возвращаем NULL - реализуем позже
    return NULL;
}

process_tree_node_t** process_tree_find_by_name(process_tree_t* tree, const char* pattern, int* count) {
    if (!tree || !tree->root || !pattern) {
        if (count) *count = 0;
        return NULL;
    }

    // Простая реализация - возвращаем NULL пока
    if (count) *count = 0;
    return NULL;
}

process_tree_node_t** process_tree_find_by_cpu(process_tree_t* tree, float min_cpu, int* count) {
    if (count) *count = 0;
    return NULL;
}

process_tree_node_t** process_tree_find_by_memory(process_tree_t* tree, float min_memory, int* count) {
    if (count) *count = 0;
    return NULL;
}

// Расчет статистики поддерева
void process_tree_calculate_subtree_stats(process_tree_node_t* node) {
    if (!node) return;

    node->subtree_cpu = node->cpu_percent;
    node->subtree_memory = node->memory_percent;

    for (int i = 0; i < node->children_count; i++) {
        process_tree_calculate_subtree_stats(node->children[i]);

        node->subtree_cpu += node->children[i]->subtree_cpu;
        node->subtree_memory += node->children[i]->subtree_memory;
        node->tree_size += node->children[i]->tree_size;
    }
}

// Сортировка процессов по CPU
void process_tree_sort_by_cpu(process_tree_node_t** nodes, int count) {
    // Простая сортировка пузырьком по убыванию CPU
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (nodes[j]->cpu_percent < nodes[j + 1]->cpu_percent) {
                process_tree_node_t* temp = nodes[j];
                nodes[j] = nodes[j + 1];
                nodes[j + 1] = temp;
            }
        }
    }
}

// Сортировка процессов по памяти
void process_tree_sort_by_memory(process_tree_node_t** nodes, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (nodes[j]->memory_percent < nodes[j + 1]->memory_percent) {
                process_tree_node_t* temp = nodes[j];
                nodes[j] = nodes[j + 1];
                nodes[j + 1] = temp;
            }
        }
    }
}

// Получение статистики группы процессов
void process_tree_get_stats(process_tree_t* tree, process_group_stats_t* stats) {
    if (!tree || !tree->root || !stats) return;

    memset(stats, 0, sizeof(process_group_stats_t));
    // Здесь должна быть реализация подсчета статистики
}

// Экспорт в DOT формат для Graphviz
int process_tree_export_dot(process_tree_t* tree, const char* filename) {
    if (!tree || !tree->root || !filename) return -1;

    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;

    fprintf(fp, "digraph ProcessTree {\n");
    fprintf(fp, "    rankdir=TB;\n");
    fprintf(fp, "    node [shape=box, style=filled];\n\n");

    // Рекурсивный обход дерева для генерации графа
    // Здесь должна быть реализация обхода

    fprintf(fp, "}\n");
    fclose(fp);

    return 0;
}

// Экспорт в JSON формат
int process_tree_export_json(process_tree_t* tree, const char* filename) {
    if (!tree || !tree->root || !filename) return -1;

    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;

    fprintf(fp, "{\n");
    fprintf(fp, "    \"timestamp\": %llu,\n", tree->timestamp);
    fprintf(fp, "    \"root_process\": {\n");

    // Здесь должна быть реализация экспорта в JSON
    fprintf(fp, "        \"pid\": %d,\n", tree->root->pid);
    fprintf(fp, "        \"name\": \"%s\",\n", tree->root->name);
    fprintf(fp, "        \"children\": []\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "}\n");

    fclose(fp);
    return 0;
}

// Экспорт в текстовый формат
int process_tree_export_txt(process_tree_t* tree, const char* filename) {
    if (!tree || !tree->root || !filename) return -1;

    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;

    fprintf(fp, "Дерево процессов - %s", ctime(&tree->timestamp));
    fprintf(fp, "═══════════════════════════════════════\n\n");

    // Здесь должна быть реализация текстового экспорта
    fclose(fp);
    return 0;
}

// Визуализация в терминале (простая версия)
void process_tree_draw_ascii(process_tree_t* tree, int max_depth) {
    if (!tree || !tree->root) return;

    printf("Дерево процессов (PID: %d, %s)\n", tree->root->pid, tree->root->name);
    printf("═══════════════════════════════════════\n");

    // Здесь должна быть реализация ASCII визуализации
    printf("Корень: %s (PID: %d)\n", tree->root->name, tree->root->pid);
}

// Утилиты

const char* process_state_to_string(char state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk sleep";
        case 'T': return "Stopped";
        case 'Z': return "Zombie";
        case 'X': return "Dead";
        default: return "Unknown";
    }
}

void format_process_info(process_tree_node_t* node, char* buffer, int buffer_size) {
    if (!node || !buffer) return;

    snprintf(buffer, buffer_size,
             "PID: %d, PPID: %d, CPU: %.1f%%, MEM: %.1f%%, State: %s, Name: %s",
             node->pid, node->ppid, node->cpu_percent, node->memory_percent,
             process_state_to_string(node->state[0]), node->name);
}

int get_process_threads(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%d/status", PROC_PATH, pid);

    FILE* fp = fopen(path, "r");
    if (!fp) return -1;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Threads:", 8) == 0) {
            int threads;
            if (sscanf(line + 8, "%d", &threads) == 1) {
                fclose(fp);
                return threads;
            }
        }
    }

    fclose(fp);
    return -1;
}

// Заглушки для остальных функций
int process_tree_refresh(process_tree_t* tree) { return process_tree_build(tree); }
process_tree_node_t** process_tree_filter(process_tree_t* tree, process_filter_t* filter, int* count) {
    if (count) *count = 0;
    return NULL;
}
void process_tree_get_node_stats(process_tree_node_t* node, process_group_stats_t* stats) { }
int process_tree_kill_subtree(process_tree_node_t* root, int signal) { return -1; }
int process_tree_suspend_subtree(process_tree_node_t* root) { return -1; }
int process_tree_resume_subtree(process_tree_node_t* root) { return -1; }
int process_tree_export_csv(process_tree_t* tree, const char* filename) { return -1; }
void process_tree_draw_interactive(process_tree_t* tree, int start_y, int height) { }
int process_tree_add_child(process_tree_node_t* parent, process_tree_node_t* child) { return 0; }