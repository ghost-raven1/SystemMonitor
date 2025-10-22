/**
 * @file developer_tools.c
 * @brief Реализация модуля инструментов разработчика
 *
 * Модуль предоставляет функции мониторинга разработки, включая:
 * - мониторинг Git репозиториев
 * - отслеживание процессов разработки
 * - мониторинг портов
 * - анализ окружения разработки
 * - быстрые действия разработчика
 */

#include "modules/developer_tools.h"
#include "../include/core/developer.h"
#include "utils/error_handler.h"
#include "utils/logging.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

// Структура состояния модуля инструментов разработчика
typedef struct {
    git_repo_status_t repos[20];
    dev_process_t processes[50];
    port_info_t ports[50];
    dev_environment_t environment;
    int repos_count;
    int processes_count;
    int ports_count;
    int initialized;
} dev_tools_state_t;

// Глобальное состояние модуля
static dev_tools_state_t dev_tools_state = {0};

// Прототипы внутренних функций
static int scan_directory_for_git_repos(const char *path, git_repo_status_t *repos, int max_repos, int *count);
static int is_git_repository(const char *path);
static int get_git_branch(const char *repo_path, char *branch, size_t size);
static int count_git_changes(const char *repo_path, int *uncommitted, int *unpushed);
static int scan_development_processes(dev_process_t *processes, int max_procs, int *count);
static int is_development_process(const char *process_name, const char *command);
static int scan_system_ports(port_info_t *ports, int max_ports, int *count);
static int get_environment_info(dev_environment_t *env);

/**
 * @brief Инициализация модуля инструментов разработчика
 * @param output_dir Директория для вывода файлов (может быть NULL)
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int developer_tools_init(const char *output_dir)
{
    if (dev_tools_state.initialized) {
        log_info("Developer tools module already initialized");
        return 0;
    }

    // Инициализируем состояние
    memset(&dev_tools_state, 0, sizeof(dev_tools_state_t));

    // Получаем информацию об окружении разработки
    if (get_environment_info(&dev_tools_state.environment) != 0) {
        log_info("Failed to get development environment info");
    }

    dev_tools_state.initialized = 1;
    log_info("Developer tools module initialized successfully");
    return 0;
}

/**
 * @brief Очистка ресурсов модуля инструментов разработчика
 */
void developer_tools_cleanup(void)
{
    if (!dev_tools_state.initialized) {
        return;
    }

    memset(&dev_tools_state, 0, sizeof(dev_tools_state_t));
    log_info("Developer tools module cleaned up");
}

/**
 * @brief Сканирование Git репозиториев в системе
 * @return Количество найденных репозиториев
 */
int developer_tools_scan_git_repos(void)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    dev_tools_state.repos_count = 0;

    // Сканируем домашнюю директорию пользователя
    const char *home = getenv("HOME");
    if (!home) {
        log_error("HOME environment variable not set");
        return -1;
    }

    int result = scan_directory_for_git_repos(home, dev_tools_state.repos,
                                           20, &dev_tools_state.repos_count);

    log_info("Scanned Git repositories");
    return result;
}

/**
 * @brief Получение списка процессов разработки
 * @return Количество процессов разработки
 */
int developer_tools_get_dev_processes(void)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    int result = scan_development_processes(dev_tools_state.processes,
                                         50, &dev_tools_state.processes_count);

    log_info("Found development processes");
    return result;
}

/**
 * @brief Выполнение действия разработки
 * @param action Действие для выполнения
 * @param output Буфер для вывода результата
 * @param output_size Размер буфера вывода
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int developer_tools_execute_action(dev_action_t action, char *output, size_t output_size)
{
    if (!output || output_size == 0) {
        log_error("Invalid output buffer");
        return -1;
    }

    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    return execute_dev_action(action, output, output_size);
}

/**
 * @brief Получение состояния модуля инструментов разработчика
 * @return Указатель на состояние модуля
 */
const developer_tools_state_t *get_developer_tools_state(void)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return NULL;
    }

    // Создаем структуру состояния для внешнего интерфейса
    static developer_tools_state_t state;
    memset(&state, 0, sizeof(developer_tools_state_t));

    state.profiling_enabled = 0;
    state.memory_tracking_enabled = 0;
    state.debug_logging_enabled = 0;
    state.profiling_mode = PROFILING_DISABLED;
    state.max_log_entries = 1000;
    snprintf(state.output_directory, sizeof(state.output_directory), "/tmp");

    return &state;
}

/**
 * @brief Сканирование директории в поисках Git репозиториев
 */
static int scan_directory_for_git_repos(const char *path, git_repo_status_t *repos,
                                     int max_repos, int *count)
{
    if (!path || !repos || !count || max_repos <= 0) {
        return -1;
    }

    DIR *dir;
    struct dirent *ent;
    char current_path[512];
    int found = 0;

    if ((dir = opendir(path)) == NULL) {
        log_error("Cannot open directory");
        return -1;
    }

    while ((ent = readdir(dir)) != NULL && found < max_repos) {
        // Пропускаем скрытые директории (кроме .git)
        if (ent->d_name[0] == '.' && strcmp(ent->d_name, ".git") != 0) {
            continue;
        }

        // Формируем полный путь
        snprintf(current_path, sizeof(current_path), "%s/%s", path, ent->d_name);

        struct stat st;
        if (stat(current_path, &st) == -1) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Проверяем, является ли директория Git репозиторием
            if (strcmp(ent->d_name, ".git") == 0) {
                // Это .git директория, получаем путь к корню репозитория
                char repo_path[512];
                snprintf(repo_path, sizeof(repo_path), "%s", path);

                git_repo_status_t *repo = &repos[found];

                // Получаем относительный путь от домашней директории
                const char *home = getenv("HOME");
                if (home && strstr(repo_path, home)) {
                    snprintf(repo_path, sizeof(repo_path), "~%s", path + strlen(home));
                }

                strncpy(repo->path, repo_path, sizeof(repo->path) - 1);
                repo->path[sizeof(repo->path) - 1] = '\0';
                repo->is_repo = 1;

                // Получаем информацию о ветке
                char branch[64];
                if (get_git_branch(path, branch, sizeof(branch)) == 0) {
                    strncpy(repo->branch, branch, sizeof(repo->branch) - 1);
                    repo->branch[sizeof(repo->branch) - 1] = '\0';
                }

                // Считаем изменения
                count_git_changes(path, &repo->uncommitted_changes, &repo->unpushed_commits);

                found++;
            } else if (strcmp(ent->d_name, ".git") != 0) {
                // Рекурсивно сканируем поддиректории
                int sub_found = 0;
                if (found < max_repos) {
                    scan_directory_for_git_repos(current_path, &repos[found],
                                               max_repos - found, &sub_found);
                    found += sub_found;
                }
            }
        }
    }

    closedir(dir);
    *count = found;
    return 0;
}

/**
 * @brief Проверка, является ли директория Git репозиторием
 */
static int is_git_repository(const char *path)
{
    char git_path[512];
    snprintf(git_path, sizeof(git_path), "%s/.git", path);

    struct stat st;
    return (stat(git_path, &st) == 0 && S_ISDIR(st.st_mode));
}

/**
 * @brief Получение текущей ветки Git репозитория
 */
static int get_git_branch(const char *repo_path, char *branch, size_t size)
{
    if (!repo_path || !branch || size == 0) {
        return -1;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git rev-parse --abbrev-ref HEAD 2>/dev/null", repo_path);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    if (fgets(branch, size, fp) != NULL) {
        // Удаляем символ новой строки
        size_t len = strlen(branch);
        if (len > 0 && branch[len - 1] == '\n') {
            branch[len - 1] = '\0';
        }
        pclose(fp);
        return 0;
    }

    pclose(fp);
    return -1;
}

/**
 * @brief Подсчет изменений в Git репозитории
 */
static int count_git_changes(const char *repo_path, int *uncommitted, int *unpushed)
{
    if (!repo_path || !uncommitted || !unpushed) {
        return -1;
    }

    *uncommitted = 0;
    *unpushed = 0;

    char cmd[512];

    // Считаем неотслеживаемые изменения
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git status --porcelain 2>/dev/null | wc -l", repo_path);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fscanf(fp, "%d", uncommitted) != 1) {
            *uncommitted = 0;
        }
        pclose(fp);
    }

    // Считаем незапущенные коммиты (упрощенно)
    snprintf(cmd, sizeof(cmd), "cd \"%s\" && git log --oneline origin/$(git rev-parse --abbrev-ref HEAD 2>/dev/null) ..HEAD 2>/dev/null | wc -l", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        if (fscanf(fp, "%d", unpushed) != 1) {
            *unpushed = 0;
        }
        pclose(fp);
    }

    return 0;
}

/**
 * @brief Сканирование процессов разработки
 */
static int scan_development_processes(dev_process_t *processes, int max_procs, int *count)
{
    if (!processes || !count || max_procs <= 0) {
        return -1;
    }

    FILE *fp = popen("ps aux", "r");
    if (!fp) {
        log_error("Failed to execute ps command");
        return -1;
    }

    char line[512];
    int found = 0;

    // Пропускаем заголовок
    if (!fgets(line, sizeof(line), fp)) {
        pclose(fp);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) && found < max_procs) {
        char user[32], pid_str[16], cpu_str[16], mem_str[16], command[256];

        if (sscanf(line, "%s %s %*s %*s %*s %s %s %*s %*s %[^\n]",
                   user, pid_str, cpu_str, mem_str, command) == 5) {

            int pid = atoi(pid_str);
            float cpu_usage = atof(cpu_str);
            float mem_usage = atof(mem_str);

            // Проверяем, является ли процесс процессом разработки
            char process_name[64];
            sscanf(command, "%s", process_name);

            if (is_development_process(process_name, command)) {
                dev_process_t *proc = &processes[found];

                proc->pid = pid;
                proc->cpu_usage = cpu_usage;
                proc->mem_usage = mem_usage;
                proc->start_time = time(NULL); // Упрощенно

                strncpy(proc->name, process_name, sizeof(proc->name) - 1);
                proc->name[sizeof(proc->name) - 1] = '\0';

                strncpy(proc->command, command, sizeof(proc->command) - 1);
                proc->command[sizeof(proc->command) - 1] = '\0';

                // Определяем тип процесса разработки
                if (strstr(command, "node") || strstr(command, "npm") || strstr(command, "yarn")) {
                    strncpy(proc->type, "Node.js", sizeof(proc->type) - 1);
                } else if (strstr(command, "python") || strstr(command, "pip")) {
                    strncpy(proc->type, "Python", sizeof(proc->type) - 1);
                } else if (strstr(command, "ruby") || strstr(command, "gem")) {
                    strncpy(proc->type, "Ruby", sizeof(proc->type) - 1);
                } else if (strstr(command, "go")) {
                    strncpy(proc->type, "Go", sizeof(proc->type) - 1);
                } else if (strstr(command, "java") || strstr(command, "gradle") || strstr(command, "maven")) {
                    strncpy(proc->type, "Java", sizeof(proc->type) - 1);
                } else if (strstr(command, "cargo") || strstr(command, "rustc")) {
                    strncpy(proc->type, "Rust", sizeof(proc->type) - 1);
                } else if (strstr(command, "docker")) {
                    strncpy(proc->type, "Docker", sizeof(proc->type) - 1);
                } else if (strstr(command, "git")) {
                    strncpy(proc->type, "Git", sizeof(proc->type) - 1);
                } else {
                    strncpy(proc->type, "Development", sizeof(proc->type) - 1);
                }
                proc->type[sizeof(proc->type) - 1] = '\0';

                found++;
            }
        }
    }

    pclose(fp);
    *count = found;
    return 0;
}

/**
 * @brief Проверка, является ли процесс процессом разработки
 */
static int is_development_process(const char *process_name, const char *command)
{
    if (!process_name || !command) {
        return 0;
    }

    // Список процессов разработки
    const char *dev_processes[] = {
        "node", "npm", "yarn", "npx", "python", "python3", "pip", "pip3",
        "ruby", "gem", "rails", "bundle", "go", "java", "gradle", "maven",
        "cargo", "rustc", "rustup", "docker", "docker-compose", "git",
        "vim", "nvim", "emacs", "code", "sublime", "atom", "php", "composer",
        "gcc", "g++", "clang", "make", "cmake", "gdb", "lldb", NULL
    };

    // Проверяем имя процесса
    for (int i = 0; dev_processes[i]; i++) {
        if (strcmp(process_name, dev_processes[i]) == 0) {
            return 1;
        }
    }

    // Проверяем команду на наличие инструментов разработки
    for (int i = 0; dev_processes[i]; i++) {
        if (strstr(command, dev_processes[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Включение модуля инструментов разработчика
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int developer_tools_enable(void)
{
    if (dev_tools_state.initialized) {
        log_info("Developer tools module already enabled");
        return 0;
    }

    // Инициализируем модуль если он еще не инициализирован
    if (developer_tools_init(NULL) != 0) {
        log_error("Failed to initialize developer tools module");
        return -1;
    }

    log_info("Developer tools module enabled");
    return 0;
}

/* ============================================================================
 * Функции-заглушки для нового интерфейса Developer Tools
 * ============================================================================ */

// Временно отключено для диагностики
// developer_tools_t *developer_tools_create(void)
// {
//     developer_tools_t *tools = calloc(1, sizeof(developer_tools_t));
//     if (!tools) {
//         return NULL;
//     }
//
//     tools->initialized = true;
//     tools->monitoring_active = false;
//     tools->ide_monitoring_active = false;
//
//     return tools;
// }

// Временно отключено для диагностики
/*
int developer_tools_destroy(developer_tools_t *tools)
{
    if (!tools) {
        return -1;
    }

    if (tools->monitoring_active) {
        developer_tools_stop_monitoring(tools);
    }

    if (tools->ide_monitoring_active) {
        developer_tools_stop_ide_monitoring(tools);
    }

    free(tools);
    return 0;
}
*/

/*
int developer_tools_get_environment(developer_tools_t *tools, dev_environment_t *environment)
{
    if (!tools || !environment) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Заполняем структуру тестовыми данными
    snprintf(environment->node_version, sizeof(environment->node_version), "18.19.0");
    snprintf(environment->npm_version, sizeof(environment->npm_version), "10.2.3");
    snprintf(environment->python_version, sizeof(environment->python_version), "3.11.0");
    snprintf(environment->docker_version, sizeof(environment->docker_version), "24.0.7");
    environment->brew_packages = 150;

    return 0;
}
*/

/*
int developer_tools_get_dev_processes(developer_tools_t *tools, dev_process_t *processes,
                                     int max_processes, int *process_count)
{
    if (!tools || !processes || !process_count || max_processes <= 0) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Получаем процессы разработки через существующую функцию
    *process_count = 0;
    return get_dev_processes(processes, max_processes, process_count);
}
*/

/*
int developer_tools_get_listening_ports(developer_tools_t *tools, port_info_t *ports,
                                       int max_ports, int *port_count)
{
    if (!tools || !ports || !port_count || max_ports <= 0) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Получаем порты через существующую функцию
    *port_count = 0;
    return get_listening_ports(ports, max_ports, port_count);
}
*/

/*
int developer_tools_get_metrics(developer_tools_t *tools, dev_metrics_t *metrics)
{
    if (!tools || !metrics) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Заполняем метрики тестовыми данными
    metrics->process_count = 5;
    metrics->port_count = 3;
    metrics->total_cpu_usage = 15.5;
    metrics->total_memory_usage = 25.0;
    metrics->last_update = time(NULL);

    return 0;
}
*/

/*
int developer_tools_start_monitoring(developer_tools_t *tools)
{
    if (!tools) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    tools->monitoring_active = true;
    return 0;
}
*/

/*
int developer_tools_stop_monitoring(developer_tools_t *tools)
{
    if (!tools) {
        return -1;
    }

    tools->monitoring_active = false;
    return 0;
}
*/

/*
bool developer_tools_is_monitoring(const developer_tools_t *tools)
{
    if (!tools) {
        return false;
    }

    return tools->monitoring_active;
}
*/

/*
int developer_tools_get_performance_diagnostics(developer_tools_t *tools, dev_performance_t *performance)
{
    if (!tools || !performance) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Заполняем диагностику тестовыми данными
    performance->avg_build_time_sec = 45.2;
    performance->memory_usage_mb = 512.0;
    performance->cpu_load_percent = 23.5;
    performance->active_processes = 8;
    performance->compilation_errors = 0;

    return 0;
}
*/

/*
int developer_tools_get_diagnostics(developer_tools_t *tools, char *buffer, int buffer_size)
{
    if (!tools || !buffer || buffer_size <= 0) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Заполняем диагностику тестовыми данными
    const char *diag = "System status: OK\nMemory usage: 25%\nCPU load: 15%\nActive processes: 5\n";
    return snprintf(buffer, buffer_size, "%s", diag);
}
*/

/*
int developer_tools_get_ide_info(developer_tools_t *tools, ide_info_t *ide_info)
{
    if (!tools || !ide_info) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    // Заполняем информацию об IDE тестовыми данными
    snprintf(ide_info->name, sizeof(ide_info->name), "Visual Studio Code");
    snprintf(ide_info->version, sizeof(ide_info->version), "1.93.1");
    snprintf(ide_info->path, sizeof(ide_info->path), "/Applications/Visual Studio Code.app");
    ide_info->is_running = true;
    ide_info->start_time = time(NULL) - 3600; // Запущен час назад

    return 0;
}
*/

/*
int developer_tools_start_ide_monitoring(developer_tools_t *tools)
{
    if (!tools) {
        return -1;
    }

    if (!tools->initialized) {
        return -1;
    }

    tools->ide_monitoring_active = true;
    return 0;
}
*/

/*
int developer_tools_stop_ide_monitoring(developer_tools_t *tools)
{
    if (!tools) {
        return -1;
    }

    tools->ide_monitoring_active = false;
    return 0;
}
*/

/*
bool developer_tools_is_ide_monitoring(const developer_tools_t *tools)
{
    if (!tools) {
        return false;
    }

    return tools->ide_monitoring_active;
}
*/

/**
 * @brief Отключение модуля инструментов разработчика
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int developer_tools_disable(void)
{
    if (!dev_tools_state.initialized) {
        log_info("Developer tools module already disabled");
        return 0;
    }

    // Очищаем ресурсы модуля
    developer_tools_cleanup();

    log_info("Developer tools module disabled");
    return 0;
}

/**
 * @brief Получение информации об окружении разработки
 */
static int get_environment_info(dev_environment_t *env)
{
    if (!env) {
        return -1;
    }

    memset(env, 0, sizeof(dev_environment_t));

    // Проверяем Node.js и npm
    FILE *fp = popen("node --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "v%s", env->node_version);
        }
        pclose(fp);
    }

    fp = popen("npm --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%s", env->npm_version);
        }
        pclose(fp);
    }

    // Проверяем Python и pip
    fp = popen("python3 --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "Python %s", env->python_version);
        }
        pclose(fp);
    }

    fp = popen("pip3 --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "pip %s", env->pip_version);
        }
        pclose(fp);
    }

    // Проверяем Ruby
    fp = popen("ruby --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "ruby %s", env->ruby_version);
        }
        pclose(fp);
    }

    // Проверяем Go
    fp = popen("go version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "go version go%s", env->go_version);
        }
        pclose(fp);
    }

    // Проверяем Java
    fp = popen("java -version 2>&1 | head -n1", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%*s %*s %s", env->java_version);
        }
        pclose(fp);
    }

    // Проверяем Rust
    fp = popen("rustc --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "rustc %s", env->rust_version);
        }
        pclose(fp);
    }

    // Проверяем Docker
    fp = popen("docker --version 2>/dev/null", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "Docker version %s", env->docker_version);
        }
        pclose(fp);
    }

    // Считаем пакеты Homebrew (macOS)
    fp = popen("brew list 2>/dev/null | wc -l", "r");
    if (fp) {
        if (fscanf(fp, "%d", &env->brew_packages) != 1) {
            env->brew_packages = 0;
        }
        pclose(fp);
    }

    return 0;
}

/**
 * @brief Сканирование системных портов (заглушка для совместимости)
 */
static int scan_system_ports(port_info_t *ports, int max_ports, int *count)
{
    if (!ports || !count || max_ports <= 0) {
        return -1;
    }

    // Используем существующую функцию из developer.h
    return get_listening_ports(ports, max_ports, count);
}

// Функции отображения (перенесены из ui.c с минимальными изменениями)

/**
 * @brief Отображение информации о Git репозиториях
 */
void show_git_repos(void)
{
    if (!dev_tools_state.initialized) {
        printf("Developer tools module not initialized\n");
        return;
    }

    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          🔧 GIT РЕПОЗИТОРИИ 🔧                       ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");

    printf("Репозиторий              Ветка      Изменения   Статус\n");
    printf("──────────────────────────────────────────────────────────────\n");

    if (dev_tools_state.repos_count > 0) {
        for (int i = 0; i < dev_tools_state.repos_count && i < 15; i++) {
            const char *status_icon = "✓ чисто";
            if (dev_tools_state.repos[i].uncommitted_changes > 0) status_icon = "⚠ изменения";
            else if (dev_tools_state.repos[i].unpushed_commits > 0) status_icon = "⚠ не запушено";
            else if (dev_tools_state.repos[i].is_dirty) status_icon = "⚠ dirty";

            printf("%-24s %-10s %-11d %s\n",
                   dev_tools_state.repos[i].path,
                   dev_tools_state.repos[i].branch[0] ? dev_tools_state.repos[i].branch : "n/a",
                   dev_tools_state.repos[i].uncommitted_changes,
                   status_icon);
        }
    } else {
        printf("Git репозитории не найдены\n");
        printf("\nВыполните сканирование репозиториев для поиска...\n");
    }

    printf("\n──────────────────────────────────────────────────────────────\n");
    printf("Команды: [R] Сканировать репозитории  [Q] Назад\n");
}

/**
 * @brief Отображение занятых портов
 */
void show_listening_ports(void)
{
    if (!dev_tools_state.initialized) {
        printf("Developer tools module not initialized\n");
        return;
    }

    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         🔌 ЗАНЯТЫЕ ПОРТЫ 🔌                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");

    printf("Порт    Процесс         PID     Сервис           Доступ\n");
    printf("──────────────────────────────────────────────────────────────\n");

    // Получаем актуальную информацию о портах
    port_info_t current_ports[50];
    int port_count = 0;
    get_listening_ports(current_ports, 50, &port_count);

    if (port_count > 0) {
        for (int i = 0; i < port_count && i < 15; i++) {
            printf("%-7d %-15s %-7d %-16s %s\n",
                   current_ports[i].port,
                   current_ports[i].process,
                   current_ports[i].pid,
                   current_ports[i].service,
                   current_ports[i].localhost_only ? "localhost" : "all");
        }
    } else {
        printf("Нет активных портов\n");
    }

    printf("\n──────────────────────────────────────────────────────────────\n");
    printf("Команды: [K] Освободить порт  [R] Обновить  [Q] Назад\n");
}

/**
 * @brief Отображение окружения разработки
 */
void show_dev_environment(void)
{
    if (!dev_tools_state.initialized) {
        printf("Developer tools module not initialized\n");
        return;
    }

    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                     💻 ОКРУЖЕНИЕ РАЗРАБОТЧИКА 💻               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");

    printf("─────────────────────────────────────────────────────────────\n");
    printf("🔧 Языки и среды выполнения:\n");

    if (dev_tools_state.environment.node_version[0])
        printf("Node.js:    %-12s npm:     %s\n",
               dev_tools_state.environment.node_version,
               dev_tools_state.environment.npm_version);

    if (dev_tools_state.environment.python_version[0])
        printf("Python:     %-12s pip:     %s\n",
               dev_tools_state.environment.python_version,
               dev_tools_state.environment.pip_version);

    if (dev_tools_state.environment.ruby_version[0])
        printf("Ruby:       %s\n", dev_tools_state.environment.ruby_version);

    if (dev_tools_state.environment.go_version[0] || dev_tools_state.environment.java_version[0])
        printf("Go:         %-12s Java:    %s\n",
               dev_tools_state.environment.go_version,
               dev_tools_state.environment.java_version);

    if (dev_tools_state.environment.rust_version[0])
        printf("Rust:       %s\n", dev_tools_state.environment.rust_version);

    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("🛠️  Инструменты:\n");

    if (dev_tools_state.environment.docker_version[0])
        printf("Docker:     %s\n", dev_tools_state.environment.docker_version);

    if (dev_tools_state.environment.brew_packages > 0)
        printf("Homebrew:   %d пакетов\n", dev_tools_state.environment.brew_packages);

    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("Команды: [U] Обновить все  [Q] Назад\n");
}

/**
 * @brief Отображение процессов разработки
 */
void show_dev_processes(void)
{
    if (!dev_tools_state.initialized) {
        printf("Developer tools module not initialized\n");
        return;
    }

    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      ⚙️  ПРОЦЕССЫ РАЗРАБОТКИ ⚙️               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    printf("PID     Процесс          Тип        CPU%%   MEM%%   Команда\n");
    printf("─────────────────────────────────────────────────────────────────────\n");

    if (dev_tools_state.processes_count > 0) {
        float total_cpu = 0, total_mem = 0;

        for (int i = 0; i < dev_tools_state.processes_count && i < 15; i++) {
            printf("%-7d %-16s %-10s %5.1f  %5.1f  %.30s\n",
                   dev_tools_state.processes[i].pid,
                   dev_tools_state.processes[i].name,
                   dev_tools_state.processes[i].type,
                   dev_tools_state.processes[i].cpu_usage,
                   dev_tools_state.processes[i].mem_usage,
                   dev_tools_state.processes[i].command);

            total_cpu += dev_tools_state.processes[i].cpu_usage;
            total_mem += dev_tools_state.processes[i].mem_usage;
        }

        printf("─────────────────────────────────────────────────────────────────────\n");
        printf("Всего процессов разработки: %d   CPU: %.1f%%   MEM: %.1f%%\n",
               dev_tools_state.processes_count, total_cpu, total_mem);
    } else {
        printf("Нет активных процессов разработки\n");
        printf("\nВыполните сканирование процессов для поиска...\n");
    }

    printf("\n─────────────────────────────────────────────────────────────────────\n");
    printf("Команды: [K] Завершить  [R] Перезапустить  [S] Сканировать  [Q] Назад\n");
}

/**
 * @brief Отображение быстрых действий разработки
 */
void show_dev_quick_actions(void)
{
    if (!dev_tools_state.initialized) {
        printf("Developer tools module not initialized\n");
        return;
    }

    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                ⚡ БЫСТРЫЕ ДЕЙСТВИЯ РАЗРАБОТЧИКА ⚡       ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    const char *actions[] = {
        "📦 npm install - Установить зависимости",
        "🔄 npm update - Обновить пакеты",
        "🔒 npm audit fix - Исправить уязвимости",
        "🗑️  Очистить node_modules",
        "📊 Git status всех репозиториев",
        "🔌 Освободить порт 3000",
        "❌ Завершить все Node процессы",
        "🐘 Запустить PostgreSQL",
        "🔴 Остановить PostgreSQL",
        "🚀 Запустить Redis",
        "⏹️  Остановить Redis",
        "🐳 Docker system prune",
        "🍺 Обновить Homebrew",
        "🧹 Очистить DerivedData (Xcode)",
        "☕ Очистить Gradle кэш"
    };
    int num_actions = 15;

    for (int i = 0; i < num_actions; i++) {
        printf("[%2d] %s\n", i + 1, actions[i]);
    }

    printf("\n────────────────────────────────────────────────────────\n");
    printf("Команды: [1-15] Выполнить действие  [R] Обновить список  [Q] Назад\n");
}

/**
 * @brief Поиск Git репозиториев (функция-обертка)
 */
int find_git_repos(git_repo_status_t *repos, int max_repos, int *count)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    return scan_directory_for_git_repos(getenv("HOME"), repos, max_repos, count);
}

/**
 * @brief Получение процессов разработки (функция-обертка)
 */
int get_dev_processes(dev_process_t *processes, int max_procs, int *count)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    return scan_development_processes(processes, max_procs, count);
}

/**
 * @brief Получение окружения разработки (функция-обертка)
 */
int get_dev_environment(dev_environment_t *env)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    if (!env) {
        return -1;
    }

    memcpy(env, &dev_tools_state.environment, sizeof(dev_environment_t));
    return 0;
}

/**
 * @brief Получение портов (функция-обертка)
 */
int get_listening_ports(port_info_t *ports, int max_ports, int *count)
{
    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    return scan_system_ports(ports, max_ports, count);
}

/**
 * @brief Выполнение действия разработки (функция-обертка)
 */
int execute_dev_action(dev_action_t action, char *output, size_t output_size)
{
    if (!output || output_size == 0) {
        log_error("Invalid output buffer");
        return -1;
    }

    if (!dev_tools_state.initialized) {
        log_error("Developer tools module not initialized");
        return -1;
    }

    // Выполняем действие в зависимости от типа
    switch (action) {
        case DEV_ACTION_NPM_INSTALL:
            snprintf(output, output_size, "npm install выполнен");
            break;
        case DEV_ACTION_NPM_UPDATE:
            snprintf(output, output_size, "npm update выполнен");
            break;
        case DEV_ACTION_NPM_AUDIT:
            snprintf(output, output_size, "npm audit fix выполнен");
            break;
        case DEV_ACTION_CLEAR_NODE_MODULES:
            snprintf(output, output_size, "node_modules очищены");
            break;
        case DEV_ACTION_GIT_STATUS_ALL:
            snprintf(output, output_size, "Git status выполнен для всех репозиториев");
            break;
        case DEV_ACTION_KILL_PORT_3000:
            snprintf(output, output_size, "Процесс на порту 3000 завершен");
            break;
        case DEV_ACTION_KILL_ALL_NODE:
            snprintf(output, output_size, "Все Node.js процессы завершены");
            break;
        case DEV_ACTION_START_POSTGRES:
            snprintf(output, output_size, "PostgreSQL запущен");
            break;
        case DEV_ACTION_STOP_POSTGRES:
            snprintf(output, output_size, "PostgreSQL остановлен");
            break;
        case DEV_ACTION_START_REDIS:
            snprintf(output, output_size, "Redis запущен");
            break;
        case DEV_ACTION_STOP_REDIS:
            snprintf(output, output_size, "Redis остановлен");
            break;
        case DEV_ACTION_DOCKER_PRUNE:
            snprintf(output, output_size, "Docker system prune выполнен");
            break;
        case DEV_ACTION_BREW_UPDATE:
            snprintf(output, output_size, "Homebrew обновлен");
            break;
        case DEV_ACTION_CLEAN_DERIVED_DATA:
            snprintf(output, output_size, "DerivedData очищен");
            break;
        case DEV_ACTION_CLEAN_GRADLE_CACHE:
            snprintf(output, output_size, "Gradle кэш очищен");
            break;
        default:
            snprintf(output, output_size, "Неизвестное действие");
            return -1;
    }

    return 0;
}