// developer.h - Developer tools for System Monitor
// Port monitoring, Git integration, databases, development environment

#ifndef DEVELOPER_H
#define DEVELOPER_H

#include <time.h>

// Port information structure
typedef struct {
    int port;
    char process[64];
    int pid;
    char user[32];
    char service[64];
    int localhost_only;
} port_info_t;

// Git repository status
typedef struct {
    char path[256];
    char branch[64];
    int uncommitted_changes;
    int unpushed_commits;
    int is_dirty;
    int is_repo;
    char last_commit[256];
} git_repo_status_t;

// Development environment info
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
} dev_environment_t;

// Database status
typedef struct {
    char name[32];
    int port;
    int is_running;
    char version[32];
    int connections;
    int databases;
    float cpu_usage;
    float mem_usage;
} database_status_t;

// Development process info
typedef struct {
    int pid;
    char name[64];
    char type[32];
    char command[256];
    float cpu_usage;
    float mem_usage;
    time_t start_time;
} dev_process_t;

// Package manager status
typedef struct {
    char name[128];
    char manager[32];  // npm, pip, brew, gem, cargo
    char current_version[32];
    char latest_version[32];
    char status[32];   // outdated, vulnerable, ok
} package_status_t;

// Localhost server info
typedef struct {
    int port;
    char process[64];
    int pid;
    char framework[32];
    int is_running;
    float response_time_ms;
    int requests_per_sec;
} localhost_server_t;

// Security issue
typedef enum {
    SECURITY_LOW,
    SECURITY_MEDIUM,
    SECURITY_HIGH,
    SECURITY_CRITICAL
} security_severity_t;

typedef struct {
    char type[32];
    char description[256];
    security_severity_t severity;
    char fix_command[256];
} security_issue_t;

// Developer quick actions
typedef enum {
    DEV_ACTION_NPM_INSTALL,
    DEV_ACTION_NPM_UPDATE,
    DEV_ACTION_NPM_AUDIT,
    DEV_ACTION_CLEAR_NODE_MODULES,
    DEV_ACTION_GIT_STATUS_ALL,
    DEV_ACTION_KILL_PORT_3000,
    DEV_ACTION_KILL_ALL_NODE,
    DEV_ACTION_START_POSTGRES,
    DEV_ACTION_STOP_POSTGRES,
    DEV_ACTION_START_REDIS,
    DEV_ACTION_STOP_REDIS,
    DEV_ACTION_DOCKER_PRUNE,
    DEV_ACTION_BREW_UPDATE,
    DEV_ACTION_CLEAN_DERIVED_DATA,
    DEV_ACTION_CLEAN_GRADLE_CACHE,
    DEV_ACTION_PIP_UPGRADE_ALL,
    DEV_ACTION_YARN_CACHE_CLEAN,
    DEV_ACTION_COMPOSER_UPDATE,
    DEV_ACTION_BUNDLER_UPDATE
} dev_action_t;

// Log monitoring
typedef struct {
    char file_path[256];
    char last_error[512];
    char last_warning[512];
    int error_count;
    int warning_count;
    time_t last_modified;
} log_monitor_t;

// Build status
typedef struct {
    char project_name[128];
    char build_tool[32];  // make, gradle, maven, npm, yarn, cargo
    int is_building;
    int last_exit_code;
    float build_time_sec;
    char last_error[256];
} build_status_t;

// API endpoint monitoring
typedef struct {
    char url[256];
    char method[16];
    int status_code;
    float response_time_ms;
    size_t response_size;
    int is_healthy;
    char last_error[256];
} api_endpoint_t;

// Code metrics
typedef struct {
    char project_path[256];
    int total_files;
    int lines_of_code;
    int comment_lines;
    int blank_lines;
    float test_coverage;
    int todo_count;
    int fixme_count;
} code_metrics_t;

// Environment variable
typedef struct {
    char name[128];
    char value[512];
    int is_sensitive;  // For passwords, tokens, etc.
} env_variable_t;

// Function declarations

// Port monitoring
int get_listening_ports(port_info_t *ports, int max_ports, int *count);
int kill_process_on_port(int port);
int check_port_available(int port);

// Git operations
int get_git_status(const char *repo_path, git_repo_status_t *status);
int find_git_repos(git_repo_status_t *repos, int max_repos, int *count);
int git_commit_all(const char *repo_path, const char *message);
int git_push(const char *repo_path);
int git_pull(const char *repo_path);

// Development environment
int get_dev_environment(dev_environment_t *env);
int check_tool_installed(const char *tool_name);
int get_env_variables(env_variable_t *vars, int max_vars, int *count);

// Database operations
int get_database_status(database_status_t *db_status, int max_dbs, int *count);
int start_database(const char *db_name);
int stop_database(const char *db_name);
int backup_database(const char *db_name, const char *backup_path);

// Process monitoring
int get_dev_processes(dev_process_t *processes, int max_procs, int *count);
int kill_dev_process(int pid);
int restart_dev_process(int pid);

// Package management
int check_outdated_packages(package_status_t *packages, int max_packages, int *count);
int update_package(const char *manager, const char *package_name);
int install_package(const char *manager, const char *package_name);
int uninstall_package(const char *manager, const char *package_name);

// Localhost servers
int get_localhost_servers(localhost_server_t *servers, int max_servers, int *count);
int test_localhost_endpoint(int port, const char *path);
int restart_localhost_server(int port);

// Security
int check_security_issues(security_issue_t *issues, int max_issues, int *count);
int fix_security_issue(const security_issue_t *issue);
int scan_for_secrets(const char *directory);

// Developer actions
int execute_dev_action(dev_action_t action, char *output, size_t output_size);
const char* get_dev_action_name(dev_action_t action);
const char* get_dev_action_description(dev_action_t action);

// Log monitoring
int monitor_log_file(const char *file_path, log_monitor_t *monitor);
int tail_log_file(const char *file_path, char *output, size_t size, int lines);
int search_log_file(const char *file_path, const char *pattern, char *output, size_t size);

// Build monitoring
int get_build_status(const char *project_path, build_status_t *status);
int start_build(const char *project_path, const char *build_command);
int stop_build(int build_pid);

// API testing
int test_api_endpoint(api_endpoint_t *endpoint);
int monitor_api_health(api_endpoint_t *endpoints, int count);
int benchmark_api(const char *url, int requests, float *avg_time);

// Code metrics
int analyze_code_metrics(const char *project_path, code_metrics_t *metrics);
int count_todos_and_fixmes(const char *project_path);
int check_code_style(const char *project_path, char *output, size_t size);

// Utility functions
const char* get_framework_by_files(const char *directory);
int detect_project_type(const char *directory);
void format_time_ago(time_t timestamp, char *output, size_t size);
void humanize_size(size_t bytes, char *output, size_t size);

#endif // DEVELOPER_H