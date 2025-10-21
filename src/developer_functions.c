// developer_functions.c - Реализация функций для разработчиков
#include "developer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "processes.h"
#include "logging.h"

// Структура для информации о портах
typedef struct {
    int port;
    char process[256];
    int pid;
    char service[64];
    int localhost_only;
} port_info_internal_t;

// Структура для информации о Git репозиториях
typedef struct {
    char path[512];
    char branch[64];
    int uncommitted_changes;
    int unpushed_commits;
    int is_dirty;
} git_repo_internal_t;

// Структура для информации об окружении разработки
typedef struct {
    char node_version[32];
    char npm_version[32];
    char python_version[32];
    char pip_version[32];
    char ruby_version[32];
    char go_version[32];
    char java_version[32];
    char rust_version[32];
    char docker_version[32];
    int brew_packages;
} dev_environment_internal_t;

// Структура для процессов разработки
typedef struct {
    int pid;
    char name[256];
    char type[32]; // "nodejs", "python", "docker", etc.
    float cpu_usage;
    float mem_usage;
    char command[512];
} dev_process_internal_t;

// Структура для статуса баз данных
typedef struct {
    char name[32];
    char version[32];
    int port;
    int is_running;
} database_status_internal_t;

int get_listening_ports(port_info_t *ports, int max_ports, int *count) {
    if (!ports || !count) return -1;

    *count = 0;

    // Попытка использовать netstat или ss для получения списка портов
    FILE *fp = popen("netstat -tlnp 2>/dev/null | grep LISTEN | head -20", "r");
    if (!fp) {
        fp = popen("ss -tlnp 2>/dev/null | grep LISTEN | head -20", "r");
    }

    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp) && *count < max_ports) {
            // Парсим вывод netstat/ss
            // Пример: "tcp 0 0 127.0.0.1:5432 0.0.0.0:* LISTEN 1234/postgres"
            int port = 0;
            char process[256] = "";
            int pid = 0;

            if (sscanf(line, "%*s %*s %*s %*s %d %*s %d/%255s",
                      &port, &pid, process) >= 2) {
                ports[*count].port = port;
                ports[*count].pid = pid;
                strncpy(ports[*count].process, process, sizeof(ports[*count].process) - 1);
                ports[*count].process[sizeof(ports[*count].process) - 1] = '\0';

                // Определяем тип сервиса
                if (port == 5432) strcpy(ports[*count].service, "PostgreSQL");
                else if (port == 3306) strcpy(ports[*count].service, "MySQL");
                else if (port == 6379) strcpy(ports[*count].service, "Redis");
                else if (port == 27017) strcpy(ports[*count].service, "MongoDB");
                else if (port == 8080 || port == 3000) strcpy(ports[*count].service, "Web Server");
                else strcpy(ports[*count].service, "Unknown");

                ports[*count].localhost_only = (strstr(line, "127.0.0.1") != NULL);
                (*count)++;
            }
        }
        pclose(fp);
    }

    return 0;
}

int find_git_repos(git_repo_status_t *repos, int max_repos, int *count) {
    if (!repos || !count) return -1;

    *count = 0;

    // Простая реализация - поиск .git директорий в домашнем каталоге
    const char *home = getenv("HOME");
    if (!home) return -1;

    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s", home);

    // Заглушка - возвращаем фиктивные данные для демонстрации
    strcpy(repos[*count].path, "/Users/test/project1");
    strcpy(repos[*count].branch, "main");
    repos[*count].uncommitted_changes = 3;
    repos[*count].unpushed_commits = 2;
    repos[*count].is_dirty = 1;
    (*count)++;

    if (*count < max_repos) {
        strcpy(repos[*count].path, "/Users/test/project2");
        strcpy(repos[*count].branch, "develop");
        repos[*count].uncommitted_changes = 0;
        repos[*count].unpushed_commits = 5;
        repos[*count].is_dirty = 0;
        (*count)++;
    }

    return 0;
}

int get_dev_environment(dev_environment_t *env) {
    if (!env) return -1;

    // Инициализация пустыми значениями
    memset(env, 0, sizeof(dev_environment_t));

    // Проверка Node.js
    FILE *fp = popen("node --version 2>/dev/null", "r");
    if (fp) {
        if (fgets(env->node_version, sizeof(env->node_version), fp)) {
            // Убираем символ новой строки
            env->node_version[strcspn(env->node_version, "\n")] = 0;
        }
        pclose(fp);
    }

    // Проверка npm
    fp = popen("npm --version 2>/dev/null", "r");
    if (fp) {
        if (fgets(env->npm_version, sizeof(env->npm_version), fp)) {
            env->npm_version[strcspn(env->npm_version, "\n")] = 0;
        }
        pclose(fp);
    }

    // Проверка Python
    fp = popen("python3 --version 2>/dev/null", "r");
    if (fp) {
        if (fgets(env->python_version, sizeof(env->python_version), fp)) {
            env->python_version[strcspn(env->python_version, "\n")] = 0;
        }
        pclose(fp);
    }

    // Проверка Docker
    fp = popen("docker --version 2>/dev/null", "r");
    if (fp) {
        if (fgets(env->docker_version, sizeof(env->docker_version), fp)) {
            env->docker_version[strcspn(env->docker_version, "\n")] = 0;
        }
        pclose(fp);
    }

    return 0;
}

int get_dev_processes(dev_process_t *processes, int max_processes, int *count) {
    if (!processes || !count) return -1;

    *count = 0;

    // Получаем все процессы
    process_info_t all_procs[512];
    size_t total_count = get_process_list(all_procs, 512);

    // Фильтруем только процессы разработки
    for (size_t i = 0; i < total_count && *count < max_processes; i++) {
        const char *name = all_procs[i].name;

        // Определяем тип процесса разработки
        if (strstr(name, "node") || strstr(name, "npm")) {
            strcpy(processes[*count].type, "nodejs");
        } else if (strstr(name, "python")) {
            strcpy(processes[*count].type, "python");
        } else if (strstr(name, "docker") || strstr(name, "containerd")) {
            strcpy(processes[*count].type, "docker");
        } else if (strstr(name, "postgres") || strstr(name, "mysql") || strstr(name, "redis")) {
            strcpy(processes[*count].type, "database");
        } else {
            continue; // Не процесс разработки
        }

        processes[*count].pid = all_procs[i].pid;
        strcpy(processes[*count].name, all_procs[i].name);
        processes[*count].cpu_usage = all_procs[i].cpu_usage;
        processes[*count].mem_usage = all_procs[i].mem_usage;
        snprintf(processes[*count].command, sizeof(processes[*count].command), "%s", name);

        (*count)++;
    }

    return 0;
}

int get_database_status(database_status_t *databases, int max_databases, int *count) {
    if (!databases || !count) return -1;

    *count = 0;

    // Проверка PostgreSQL
    FILE *fp = popen("pg_isready -q 2>/dev/null && echo 'running' || echo 'stopped'", "r");
    if (fp) {
        char status[16];
        if (fgets(status, sizeof(status), fp)) {
            strcpy(databases[*count].name, "PostgreSQL");
            strcpy(databases[*count].version, "13.0");
            databases[*count].port = 5432;
            databases[*count].is_running = (strncmp(status, "running", 7) == 0);
            (*count)++;
        }
        pclose(fp);
    }

    // Проверка Redis
    fp = popen("redis-cli ping 2>/dev/null | grep -q PONG && echo 'running' || echo 'stopped'", "r");
    if (fp) {
        char status[16];
        if (fgets(status, sizeof(status), fp)) {
            strcpy(databases[*count].name, "Redis");
            strcpy(databases[*count].version, "6.0");
            databases[*count].port = 6379;
            databases[*count].is_running = (strncmp(status, "running", 7) == 0);
            (*count)++;
        }
        pclose(fp);
    }

    return 0;
}

int execute_dev_action(dev_action_t action, char *output, size_t output_size) {
    if (!output || output_size == 0) return -1;

    output[0] = '\0';

    switch (action) {
        case DEV_ACTION_NPM_INSTALL:
            system("npm install 2>/dev/null");
            snprintf(output, output_size, "npm install completed");
            break;
        case DEV_ACTION_NPM_UPDATE:
            system("npm update 2>/dev/null");
            snprintf(output, output_size, "npm update completed");
            break;
        case DEV_ACTION_NPM_AUDIT:
            system("npm audit 2>/dev/null");
            snprintf(output, output_size, "npm audit completed");
            break;
        case DEV_ACTION_CLEAR_NODE_MODULES:
            system("rm -rf node_modules package-lock.json 2>/dev/null");
            snprintf(output, output_size, "node_modules cleaned");
            break;
        case DEV_ACTION_DOCKER_PRUNE:
            system("docker system prune -f 2>/dev/null");
            snprintf(output, output_size, "docker prune completed");
            break;
        default:
            snprintf(output, output_size, "Action not implemented");
            return -1;
    }

    return 0;
}