// developer_impl.c - Working implementation of developer tools
// Simplified version without external dependencies

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <signal.h>
#include "developer.h"

// Get listening ports using lsof
int get_listening_ports(port_info_t *ports, int max_ports, int *count) {
    if (!ports || !count || max_ports <= 0) return -1;
    
    *count = 0;
    
    FILE *fp = popen("lsof -iTCP -sTCP:LISTEN -n -P 2>/dev/null | tail -n +2", "r");
    if (!fp) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), fp) && *count < max_ports) {
        port_info_t *port = &ports[*count];
        char cmd[64], user[32];
        int pid;
        char *port_str;
        
        // Parse lsof output
        if (sscanf(line, "%63s %d %31s", cmd, &pid, user) >= 3) {
            // Find port number in the line
            port_str = strstr(line, ":");
            if (port_str && *(port_str+1)) {
                int port_num = 0;
                char *end_ptr = port_str + 1;
                
                // Extract port number
                while (*end_ptr && isdigit(*end_ptr)) {
                    port_num = port_num * 10 + (*end_ptr - '0');
                    end_ptr++;
                }
                
                if (port_num > 0) {
                    port->port = port_num;
                    port->pid = pid;
                    strncpy(port->process, cmd, sizeof(port->process) - 1);
                    strncpy(port->user, user, sizeof(port->user) - 1);
                    
                    // Check if localhost only
                    port->localhost_only = (strstr(line, "127.0.0.1") || strstr(line, "localhost")) ? 1 : 0;
                    
                    // Identify common services
                    switch(port_num) {
                        case 3000:
                        case 3001:
                        case 4200:
                        case 8080:
                        case 8000:
                        case 5000:
                            strncpy(port->service, "Dev Server", sizeof(port->service) - 1);
                            break;
                        case 5432:
                            strncpy(port->service, "PostgreSQL", sizeof(port->service) - 1);
                            break;
                        case 3306:
                            strncpy(port->service, "MySQL", sizeof(port->service) - 1);
                            break;
                        case 27017:
                            strncpy(port->service, "MongoDB", sizeof(port->service) - 1);
                            break;
                        case 6379:
                            strncpy(port->service, "Redis", sizeof(port->service) - 1);
                            break;
                        default:
                            strncpy(port->service, "Unknown", sizeof(port->service) - 1);
                            break;
                    }
                    
                    (*count)++;
                }
            }
        }
    }
    
    pclose(fp);
    return 0;
}

// Kill process on port
int kill_process_on_port(int port) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lsof -ti:%d 2>/dev/null | xargs kill -9 2>/dev/null", port);
    return system(cmd);
}

// Check if port is available
int check_port_available(int port) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lsof -i:%d 2>/dev/null | grep -q LISTEN", port);
    return system(cmd) != 0; // Returns 1 if available, 0 if in use
}

// Get git repository status
int get_git_status(const char *repo_path, git_repo_status_t *status) {
    if (!repo_path || !status) return -1;
    
    memset(status, 0, sizeof(git_repo_status_t));
    
    // Check if it's a git repo
    char git_dir[512];
    snprintf(git_dir, sizeof(git_dir), "%s/.git", repo_path);
    struct stat st;
    if (stat(git_dir, &st) != 0) {
        status->is_repo = 0;
        return -1;
    }
    
    status->is_repo = 1;
    strncpy(status->path, repo_path, sizeof(status->path) - 1);
    
    // Get current branch
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cd '%s' && git branch --show-current 2>/dev/null", repo_path);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(status->branch, sizeof(status->branch), fp)) {
            status->branch[strcspn(status->branch, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Get uncommitted changes count
    snprintf(cmd, sizeof(cmd), "cd '%s' && git status --porcelain 2>/dev/null | wc -l", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        fscanf(fp, "%d", &status->uncommitted_changes);
        pclose(fp);
    }
    
    // Get unpushed commits
    snprintf(cmd, sizeof(cmd), "cd '%s' && git log @{u}.. --oneline 2>/dev/null | wc -l", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        fscanf(fp, "%d", &status->unpushed_commits);
        pclose(fp);
    }
    
    // Get last commit info
    snprintf(cmd, sizeof(cmd), "cd '%s' && git log -1 --format='%%h|%%s|%%ar' 2>/dev/null", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;
            strncpy(status->last_commit, line, sizeof(status->last_commit) - 1);
        }
        pclose(fp);
    }
    
    // Check if dirty
    snprintf(cmd, sizeof(cmd), "cd '%s' && git diff --quiet 2>/dev/null", repo_path);
    status->is_dirty = (system(cmd) != 0);
    
    return 0;
}

// Find git repositories
int find_git_repos(git_repo_status_t *repos, int max_repos, int *count) {
    if (!repos || !count || max_repos <= 0) return -1;
    
    *count = 0;
    struct passwd *pw = getpwuid(getuid());
    const char *home = pw ? pw->pw_dir : getenv("HOME");
    if (!home) return -1;
    
    // Common development directories to search
    const char *search_dirs[] = {
        "Documents/GitHub",
        "Documents/Projects", 
        "Developer",
        "Projects",
        "Code",
        "src",
        "repos",
        "work",
        NULL
    };
    
    for (int i = 0; search_dirs[i] && *count < max_repos; i++) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", home, search_dirs[i]);
        
        DIR *dir = opendir(full_path);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) && *count < max_repos) {
            if (entry->d_name[0] == '.') continue;
            
            char repo_path[512];
            snprintf(repo_path, sizeof(repo_path), "%s/%s", full_path, entry->d_name);
            
            // Check if it has .git directory
            char git_path[512];
            snprintf(git_path, sizeof(git_path), "%s/.git", repo_path);
            struct stat st;
            if (stat(git_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                get_git_status(repo_path, &repos[*count]);
                (*count)++;
            }
        }
        closedir(dir);
    }
    
    return 0;
}

// Get development environment info
int get_dev_environment(dev_environment_t *env) {
    if (!env) return -1;
    
    memset(env, 0, sizeof(dev_environment_t));
    
    // Node.js version
    FILE *fp = popen("node --version 2>/dev/null", "r");
    if (fp) {
        fgets(env->node_version, sizeof(env->node_version), fp);
        env->node_version[strcspn(env->node_version, "\n")] = 0;
        pclose(fp);
    }
    
    // npm version
    fp = popen("npm --version 2>/dev/null", "r");
    if (fp) {
        fgets(env->npm_version, sizeof(env->npm_version), fp);
        env->npm_version[strcspn(env->npm_version, "\n")] = 0;
        pclose(fp);
    }
    
    // Python version
    fp = popen("python3 --version 2>&1 | cut -d' ' -f2", "r");
    if (fp) {
        fgets(env->python_version, sizeof(env->python_version), fp);
        env->python_version[strcspn(env->python_version, "\n")] = 0;
        pclose(fp);
    }
    
    // Ruby version
    fp = popen("ruby --version 2>/dev/null | cut -d' ' -f2", "r");
    if (fp) {
        fgets(env->ruby_version, sizeof(env->ruby_version), fp);
        env->ruby_version[strcspn(env->ruby_version, "\n")] = 0;
        pclose(fp);
    }
    
    // Go version
    fp = popen("go version 2>/dev/null | awk '{print $3}' | sed 's/go//'", "r");
    if (fp) {
        fgets(env->go_version, sizeof(env->go_version), fp);
        env->go_version[strcspn(env->go_version, "\n")] = 0;
        pclose(fp);
    }
    
    // Java version
    fp = popen("java -version 2>&1 | head -1 | cut -d'\"' -f2", "r");
    if (fp) {
        fgets(env->java_version, sizeof(env->java_version), fp);
        env->java_version[strcspn(env->java_version, "\n")] = 0;
        pclose(fp);
    }
    
    // Docker version
    fp = popen("docker --version 2>/dev/null | awk '{print $3}' | sed 's/,//'", "r");
    if (fp) {
        fgets(env->docker_version, sizeof(env->docker_version), fp);
        env->docker_version[strcspn(env->docker_version, "\n")] = 0;
        pclose(fp);
    }
    
    // Homebrew packages count
    fp = popen("brew list 2>/dev/null | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &env->brew_packages);
        pclose(fp);
    }
    
    return 0;
}

// Get database status
int get_database_status(database_status_t *db_status, int max_dbs, int *count) {
    if (!db_status || !count || max_dbs <= 0) return -1;
    
    *count = 0;
    
    // Check PostgreSQL
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        strncpy(db->name, "PostgreSQL", sizeof(db->name) - 1);
        db->port = 5432;
        
        // Check if running
        FILE *fp = popen("pg_isready 2>/dev/null | grep -q 'accepting connections' && echo 'running'", "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp)) {
                db->is_running = 1;
                
                // Get version
                FILE *ver_fp = popen("postgres --version 2>/dev/null | awk '{print $3}'", "r");
                if (ver_fp) {
                    fgets(db->version, sizeof(db->version), ver_fp);
                    db->version[strcspn(db->version, "\n")] = 0;
                    pclose(ver_fp);
                }
            }
            pclose(fp);
        }
        (*count)++;
    }
    
    // Check MySQL
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        strncpy(db->name, "MySQL", sizeof(db->name) - 1);
        db->port = 3306;
        
        FILE *fp = popen("mysqladmin ping 2>/dev/null | grep -q alive && echo 'running'", "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp)) {
                db->is_running = 1;
            }
            pclose(fp);
        }
        (*count)++;
    }
    
    // Check Redis
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        strncpy(db->name, "Redis", sizeof(db->name) - 1);
        db->port = 6379;
        
        FILE *fp = popen("redis-cli ping 2>/dev/null", "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp) && strstr(buf, "PONG")) {
                db->is_running = 1;
            }
            pclose(fp);
        }
        (*count)++;
    }
    
    // Check MongoDB
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        strncpy(db->name, "MongoDB", sizeof(db->name) - 1);
        db->port = 27017;
        
        FILE *fp = popen("pgrep mongod >/dev/null && echo 'running'", "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp)) {
                db->is_running = 1;
            }
            pclose(fp);
        }
        (*count)++;
    }
    
    return 0;
}

// Get development processes
int get_dev_processes(dev_process_t *processes, int max_procs, int *count) {
    if (!processes || !count || max_procs <= 0) return -1;
    
    *count = 0;
    
    // List of development-related process names
    const char *dev_names[] = {
        "node", "npm", "yarn", "webpack", "vite",
        "python", "python3", "pip", "django", "flask",
        "ruby", "rails", "bundler",
        "java", "gradle", "maven", 
        "go", "cargo", "rustc",
        "docker", "nginx", "apache",
        "postgres", "mysql", "mongod", "redis",
        NULL
    };
    
    // Get all processes and filter
    FILE *fp = popen("ps aux", "r");
    if (!fp) return -1;
    
    char line[512];
    // Skip header
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp) && *count < max_procs) {
        char user[32], cmd[256];
        int pid;
        float cpu, mem;
        
        if (sscanf(line, "%31s %d %f %f %*s %*s %*s %*s %*s %*s %255[^\n]",
                   user, &pid, &cpu, &mem, cmd) >= 5) {
            
            // Check if it's a development process
            for (int i = 0; dev_names[i]; i++) {
                if (strstr(cmd, dev_names[i])) {
                    dev_process_t *proc = &processes[*count];
                    proc->pid = pid;
                    proc->cpu_usage = cpu;
                    proc->mem_usage = mem;
                    
                    // Extract process name
                    char *name = strrchr(cmd, '/');
                    if (name) name++;
                    else name = cmd;
                    
                    char *space = strchr(name, ' ');
                    if (space) *space = '\0';
                    
                    strncpy(proc->name, name, sizeof(proc->name) - 1);
                    strncpy(proc->type, dev_names[i], sizeof(proc->type) - 1);
                    strncpy(proc->command, cmd, sizeof(proc->command) - 1);
                    
                    (*count)++;
                    break;
                }
            }
        }
    }
    
    pclose(fp);
    return 0;
}

// Execute developer action
int execute_dev_action(dev_action_t action, char *output, size_t output_size) {
    if (!output || output_size == 0) return -1;
    
    const char *cmd = NULL;
    
    switch(action) {
        case DEV_ACTION_NPM_INSTALL:
            cmd = "npm install 2>&1 | tail -5";
            break;
        case DEV_ACTION_NPM_UPDATE:
            cmd = "npm update 2>&1 | tail -5";
            break;
        case DEV_ACTION_NPM_AUDIT:
            cmd = "npm audit fix 2>&1 | tail -5";
            break;
        case DEV_ACTION_CLEAR_NODE_MODULES:
            cmd = "rm -rf node_modules package-lock.json && echo 'node_modules cleared'";
            break;
        case DEV_ACTION_KILL_PORT_3000:
            cmd = "lsof -ti:3000 | xargs kill -9 2>/dev/null && echo 'Port 3000 freed' || echo 'Port 3000 not in use'";
            break;
        case DEV_ACTION_KILL_ALL_NODE:
            cmd = "killall node 2>/dev/null && echo 'Node processes killed' || echo 'No node processes'";
            break;
        case DEV_ACTION_START_POSTGRES:
            cmd = "brew services start postgresql 2>/dev/null && echo 'PostgreSQL started' || echo 'PostgreSQL start failed'";
            break;
        case DEV_ACTION_STOP_POSTGRES:
            cmd = "brew services stop postgresql 2>/dev/null && echo 'PostgreSQL stopped' || echo 'PostgreSQL stop failed'";
            break;
        case DEV_ACTION_START_REDIS:
            cmd = "redis-server --daemonize yes 2>/dev/null && echo 'Redis started' || echo 'Redis start failed'";
            break;
        case DEV_ACTION_STOP_REDIS:
            cmd = "redis-cli shutdown 2>/dev/null && echo 'Redis stopped' || echo 'Redis stop failed'";
            break;
        case DEV_ACTION_DOCKER_PRUNE:
            cmd = "docker system prune -f 2>&1 | tail -5";
            break;
        case DEV_ACTION_BREW_UPDATE:
            cmd = "brew update 2>&1 | tail -5";
            break;
        default:
            snprintf(output, output_size, "Unknown action");
            return -1;
    }
    
    if (cmd) {
        FILE *fp = popen(cmd, "r");
        if (fp) {
            size_t n = fread(output, 1, output_size - 1, fp);
            output[n] = '\0';
            pclose(fp);
            return 0;
        }
    }
    
    snprintf(output, output_size, "Execution failed");
    return -1;
}

// Get localhost servers
int get_localhost_servers(localhost_server_t *servers, int max_servers, int *count) {
    if (!servers || !count || max_servers <= 0) return -1;
    
    *count = 0;
    
    // Common development ports
    int ports[] = {3000, 3001, 4200, 5000, 8000, 8080, 8081, 9000, 0};
    
    for (int i = 0; ports[i] && *count < max_servers; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "lsof -i:%d -P -n 2>/dev/null | grep LISTEN", ports[i]);
        
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char line[256];
            if (fgets(line, sizeof(line), fp)) {
                localhost_server_t *server = &servers[*count];
                server->port = ports[i];
                server->is_running = 1;
                
                // Parse process name
                char process[64];
                int pid;
                if (sscanf(line, "%63s %d", process, &pid) >= 2) {
                    strncpy(server->process, process, sizeof(server->process) - 1);
                    server->pid = pid;
                }
                
                // Determine framework
                if (strstr(process, "node")) {
                    strncpy(server->framework, "Node.js", sizeof(server->framework) - 1);
                } else if (strstr(process, "python")) {
                    strncpy(server->framework, "Python", sizeof(server->framework) - 1);
                } else {
                    strncpy(server->framework, "Unknown", sizeof(server->framework) - 1);
                }
                
                (*count)++;
            }
            pclose(fp);
        }
    }
    
    return 0;
}

// Check outdated packages (simplified)
int check_outdated_packages(package_status_t *packages, int max_packages, int *count) {
    if (!packages || !count || max_packages <= 0) return -1;
    
    *count = 0;
    
    // Check npm outdated
    FILE *fp = popen("npm outdated 2>/dev/null | tail -n +2 | head -5", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp) && *count < max_packages) {
            package_status_t *pkg = &packages[*count];
            char name[64], current[32], wanted[32], latest[32];
            
            if (sscanf(line, "%63s %31s %31s %31s", name, current, wanted, latest) >= 4) {
                strncpy(pkg->name, name, sizeof(pkg->name) - 1);
                strncpy(pkg->current_version, current, sizeof(pkg->current_version) - 1);
                strncpy(pkg->latest_version, latest, sizeof(pkg->latest_version) - 1);
                strncpy(pkg->manager, "npm", sizeof(pkg->manager) - 1);
                strncpy(pkg->status, "outdated", sizeof(pkg->status) - 1);
                (*count)++;
            }
        }
        pclose(fp);
    }
    
    return 0;
}

// Utility function implementations
const char* get_dev_action_name(dev_action_t action) {
    switch(action) {
        case DEV_ACTION_NPM_INSTALL: return "npm install";
        case DEV_ACTION_NPM_UPDATE: return "npm update";
        case DEV_ACTION_NPM_AUDIT: return "npm audit fix";
        case DEV_ACTION_CLEAR_NODE_MODULES: return "Clear node_modules";
        case DEV_ACTION_KILL_PORT_3000: return "Free port 3000";
        case DEV_ACTION_KILL_ALL_NODE: return "Kill all Node";
        case DEV_ACTION_START_POSTGRES: return "Start PostgreSQL";
        case DEV_ACTION_STOP_POSTGRES: return "Stop PostgreSQL";
        case DEV_ACTION_START_REDIS: return "Start Redis";
        case DEV_ACTION_STOP_REDIS: return "Stop Redis";
        case DEV_ACTION_DOCKER_PRUNE: return "Docker prune";
        case DEV_ACTION_BREW_UPDATE: return "Brew update";
        default: return "Unknown";
    }
}

const char* get_dev_action_description(dev_action_t action) {
    switch(action) {
        case DEV_ACTION_NPM_INSTALL: return "Install npm dependencies";
        case DEV_ACTION_NPM_UPDATE: return "Update npm packages";
        case DEV_ACTION_NPM_AUDIT: return "Fix security vulnerabilities";
        case DEV_ACTION_CLEAR_NODE_MODULES: return "Remove and reinstall dependencies";
        case DEV_ACTION_KILL_PORT_3000: return "Kill process on port 3000";
        case DEV_ACTION_KILL_ALL_NODE: return "Terminate all Node.js processes";
        case DEV_ACTION_START_POSTGRES: return "Start PostgreSQL database";
        case DEV_ACTION_STOP_POSTGRES: return "Stop PostgreSQL database";
        case DEV_ACTION_START_REDIS: return "Start Redis server";
        case DEV_ACTION_STOP_REDIS: return "Stop Redis server";
        case DEV_ACTION_DOCKER_PRUNE: return "Clean up Docker resources";
        case DEV_ACTION_BREW_UPDATE: return "Update Homebrew packages";
        default: return "Unknown action";
    }
}