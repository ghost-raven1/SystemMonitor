// developer.c - Developer tools for System Monitor
// Port monitoring, Git integration, databases, development environment

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
#include "developer.h"

// Port monitoring - find what's listening on which ports
int get_listening_ports(port_info_t *ports, int max_ports, int *count) {
    if (!ports || !count || max_ports <= 0) return -1;
    
    *count = 0;
    
    // Use lsof to get listening ports with process info
    FILE *fp = popen("lsof -iTCP -sTCP:LISTEN -n -P 2>/dev/null | tail -n +2", "r");
    if (!fp) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), fp) && *count < max_ports) {
        port_info_t *port = &ports[*count];
        char cmd[64], user[32], type[16], device[32], node[64], name[128];
        int pid;
        
        if (sscanf(line, "%63s %d %31s %*s %15s %31s %*s %63s %127[^\n]",
                   cmd, &pid, user, type, device, node, name) >= 6) {
            
            // Extract port number from name field (e.g., "*:3000" or "localhost:8080")
            char *port_str = strrchr(name, ':');
            if (port_str) {
                int port_num = atoi(port_str + 1);
                port->port = port_num;
                snprintf(port->process, sizeof(port->process), "%s", cmd);
                port->pid = pid;
                snprintf(port->user, sizeof(port->user), "%s", user);
                
                // Determine if it's localhost only or all interfaces
                if (strstr(name, "*:") || strstr(name, "0.0.0.0:")) {
                    port->localhost_only = 0;
                } else {
                    port->localhost_only = 1;
                }
                
                // Identify common services
                switch(port_num) {
                    case 3000:
                    case 3001:
                    case 4200:
                    case 8080:
                    case 8000:
                    case 5000:
                        snprintf(port->service, sizeof(port->service), "Dev Server");
                        break;
                    case 5432:
                        snprintf(port->service, sizeof(port->service), "PostgreSQL");
                        break;
                    case 3306:
                        snprintf(port->service, sizeof(port->service), "MySQL");
                        break;
                    case 27017:
                        snprintf(port->service, sizeof(port->service), "MongoDB");
                        break;
                    case 6379:
                        snprintf(port->service, sizeof(port->service), "Redis");
                        break;
                    case 9200:
                        snprintf(port->service, sizeof(port->service), "Elasticsearch");
                        break;
                    case 5672:
                        snprintf(port->service, sizeof(port->service), "RabbitMQ");
                        break;
                    case 8081:
                        snprintf(port->service, sizeof(port->service), "Admin Panel");
                        break;
                    default:
                        snprintf(port->service, sizeof(port->service), "Unknown");
                        break;
                }
                
                (*count)++;
            }
        }
    }
    
    pclose(fp);
    return 0;
}

// Git repository status
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
    snprintf(status->path, sizeof(status->path), "%s", repo_path);
    
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
    snprintf(cmd, sizeof(cmd), "cd '%s' && git cherry -v 2>/dev/null | wc -l", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        fscanf(fp, "%d", &status->unpushed_commits);
        pclose(fp);
    }
    
    // Get last commit info
    snprintf(cmd, sizeof(cmd), "cd '%s' && git log -1 --format='%%h|%%an|%%ar|%%s' 2>/dev/null", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            char hash[16], author[64], time[64], message[128];
            if (sscanf(line, "%15[^|]|%63[^|]|%63[^|]|%127[^\n]",
                       hash, author, time, message) >= 4) {
                snprintf(status->last_commit, sizeof(status->last_commit),
                         "%s - %s (%s)", hash, message, time);
            }
        }
        pclose(fp);
    }
    
    // Check if dirty
    snprintf(cmd, sizeof(cmd), "cd '%s' && git diff --quiet 2>/dev/null || echo 'dirty'", repo_path);
    fp = popen(cmd, "r");
    if (fp) {
        char buf[16];
        if (fgets(buf, sizeof(buf), fp)) {
            status->is_dirty = 1;
        }
        pclose(fp);
    }
    
    return 0;
}

// Find all git repositories in home directory
int find_git_repos(git_repo_status_t *repos, int max_repos, int *count) {
    if (!repos || !count || max_repos <= 0) return -1;
    
    *count = 0;
    struct passwd *pw = getpwuid(getuid());
    const char *home = pw->pw_dir;
    
    // Common development directories
    const char *dev_dirs[] = {
        "~/Documents/GitHub",
        "~/Documents/Projects",
        "~/Developer",
        "~/Projects",
        "~/Code",
        "~/src",
        "~/repos",
        "~/work",
        NULL
    };
    
    for (int i = 0; dev_dirs[i] && *count < max_repos; i++) {
        char expanded_path[512];
        if (dev_dirs[i][0] == '~') {
            snprintf(expanded_path, sizeof(expanded_path), "%s%s", home, dev_dirs[i] + 1);
        } else {
            snprintf(expanded_path, sizeof(expanded_path), "%s", dev_dirs[i]);
        }
        
        DIR *dir = opendir(expanded_path);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) && *count < max_repos) {
            if (entry->d_name[0] == '.') continue;
            
            char repo_path[512];
            snprintf(repo_path, sizeof(repo_path), "%s/%s", expanded_path, entry->d_name);
            
            git_repo_status_t temp_status;
            if (get_git_status(repo_path, &temp_status) == 0 && temp_status.is_repo) {
                memcpy(&repos[*count], &temp_status, sizeof(git_repo_status_t));
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
        if (fgets(env->node_version, sizeof(env->node_version), fp)) {
            env->node_version[strcspn(env->node_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // npm version
    fp = popen("npm --version 2>/dev/null", "r");
    if (fp) {
        if (fgets(env->npm_version, sizeof(env->npm_version), fp)) {
            env->npm_version[strcspn(env->npm_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Python version
    fp = popen("python3 --version 2>&1 | cut -d' ' -f2", "r");
    if (fp) {
        if (fgets(env->python_version, sizeof(env->python_version), fp)) {
            env->python_version[strcspn(env->python_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // pip version
    fp = popen("pip3 --version 2>/dev/null | cut -d' ' -f2", "r");
    if (fp) {
        if (fgets(env->pip_version, sizeof(env->pip_version), fp)) {
            env->pip_version[strcspn(env->pip_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Ruby version
    fp = popen("ruby --version 2>/dev/null | cut -d' ' -f2", "r");
    if (fp) {
        if (fgets(env->ruby_version, sizeof(env->ruby_version), fp)) {
            env->ruby_version[strcspn(env->ruby_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Go version
    fp = popen("go version 2>/dev/null | cut -d' ' -f3 | sed 's/go//'", "r");
    if (fp) {
        if (fgets(env->go_version, sizeof(env->go_version), fp)) {
            env->go_version[strcspn(env->go_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Java version
    fp = popen("java -version 2>&1 | head -1 | cut -d'\"' -f2", "r");
    if (fp) {
        if (fgets(env->java_version, sizeof(env->java_version), fp)) {
            env->java_version[strcspn(env->java_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Rust version
    fp = popen("rustc --version 2>/dev/null | cut -d' ' -f2", "r");
    if (fp) {
        if (fgets(env->rust_version, sizeof(env->rust_version), fp)) {
            env->rust_version[strcspn(env->rust_version, "\n")] = 0;
        }
        pclose(fp);
    }
    
    // Docker version
    fp = popen("docker --version 2>/dev/null | cut -d' ' -f3 | sed 's/,//'", "r");
    if (fp) {
        if (fgets(env->docker_version, sizeof(env->docker_version), fp)) {
            env->docker_version[strcspn(env->docker_version, "\n")] = 0;
        }
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

// Database status checker
int get_database_status(database_status_t *db_status, int max_dbs, int *count) {
    if (!db_status || !count || max_dbs <= 0) return -1;
    
    *count = 0;
    
    // Check PostgreSQL
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        snprintf(db->name, sizeof(db->name), "PostgreSQL");
        db->port = 5432;
        
        FILE *fp = popen("pg_isready 2>/dev/null", "r");
        if (fp) {
            char buf[128];
            if (fgets(buf, sizeof(buf), fp)) {
                db->is_running = (strstr(buf, "accepting connections") != NULL);
            }
            pclose(fp);
            
            if (db->is_running) {
                // Get version
                fp = popen("psql --version 2>/dev/null | cut -d' ' -f3", "r");
                if (fp) {
                    if (fgets(db->version, sizeof(db->version), fp)) {
                        db->version[strcspn(db->version, "\n")] = 0;
                    }
                    pclose(fp);
                }
                
                // Get database count
                fp = popen("psql -U $USER -lqt 2>/dev/null | wc -l", "r");
                if (fp) {
                    fscanf(fp, "%d", &db->databases);
                    pclose(fp);
                }
            }
        }
        (*count)++;
    }
    
    // Check MySQL
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        snprintf(db->name, sizeof(db->name), "MySQL");
        db->port = 3306;
        
        FILE *fp = popen("mysqladmin ping 2>/dev/null | grep -q alive && echo 'running'", "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp)) {
                db->is_running = 1;
            }
            pclose(fp);
            
            if (db->is_running) {
                // Get version
                fp = popen("mysql --version 2>/dev/null | cut -d' ' -f6 | sed 's/,//'", "r");
                if (fp) {
                    if (fgets(db->version, sizeof(db->version), fp)) {
                        db->version[strcspn(db->version, "\n")] = 0;
                    }
                    pclose(fp);
                }
            }
        }
        (*count)++;
    }
    
    // Check MongoDB
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        snprintf(db->name, sizeof(db->name), "MongoDB");
        db->port = 27017;
        
        FILE *fp = popen("mongosh --eval 'db.version()' --quiet 2>/dev/null", "r");
        if (fp) {
            if (fgets(db->version, sizeof(db->version), fp)) {
                db->version[strcspn(db->version, "\n")] = 0;
                db->is_running = 1;
            }
            pclose(fp);
        }
        (*count)++;
    }
    
    // Check Redis
    if (*count < max_dbs) {
        database_status_t *db = &db_status[*count];
        snprintf(db->name, sizeof(db->name), "Redis");
        db->port = 6379;
        
        FILE *fp = popen("redis-cli ping 2>/dev/null", "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp) && strstr(buf, "PONG")) {
                db->is_running = 1;
                
                // Get version
                FILE *ver_fp = popen("redis-server --version 2>/dev/null | cut -d'=' -f2 | cut -d' ' -f1", "r");
                if (ver_fp) {
                    if (fgets(db->version, sizeof(db->version), ver_fp)) {
                        db->version[strcspn(db->version, "\n")] = 0;
                    }
                    pclose(ver_fp);
                }
            }
            pclose(fp);
        }
        (*count)++;
    }
    
    return 0;
}

// Get running dev processes
int get_dev_processes(dev_process_t *processes, int max_procs, int *count) {
    if (!processes || !count || max_procs <= 0) return -1;
    
    *count = 0;
    
    // Define development-related process patterns
    const char *dev_patterns[] = {
        "node", "npm", "yarn", "webpack", "vite", "next",
        "python", "pip", "django", "flask", "jupyter",
        "ruby", "rails", "bundler",
        "java", "gradle", "maven",
        "go", "cargo", "rustc",
        "php", "composer", "laravel",
        "dotnet", "mono",
        "docker", "docker-compose",
        "nginx", "apache", "httpd",
        "postgres", "mysql", "mongod", "redis",
        "elasticsearch", "kibana", "logstash",
        "code", "vim", "nvim", "emacs",
        NULL
    };
    
    for (int i = 0; dev_patterns[i] && *count < max_procs; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), 
                "ps aux | grep -i '%s' | grep -v grep | head -5", 
                dev_patterns[i]);
        
        FILE *fp = popen(cmd, "r");
        if (!fp) continue;
        
        char line[512];
        while (fgets(line, sizeof(line), fp) && *count < max_procs) {
            dev_process_t *proc = &processes[*count];
            char user[32], command[256];
            int pid;
            float cpu, mem;
            
            if (sscanf(line, "%31s %d %f %f %*s %*s %*s %*s %*s %*s %255[^\n]",
                       user, &pid, &cpu, &mem, command) >= 5) {
                
                proc->pid = pid;
                proc->cpu_usage = cpu;
                proc->mem_usage = mem;
                snprintf(proc->type, sizeof(proc->type), "%s", dev_patterns[i]);
                
                // Extract just the command name
                char *cmd_name = command;
                char *space = strchr(command, ' ');
                if (space) *space = '\0';
                
                char *last_slash = strrchr(cmd_name, '/');
                if (last_slash) cmd_name = last_slash + 1;
                
                snprintf(proc->name, sizeof(proc->name), "%s", cmd_name);
                snprintf(proc->command, sizeof(proc->command), "%s", command);
                
                (*count)++;
            }
        }
        pclose(fp);
    }
    
    return 0;
}

// Package manager operations
int check_outdated_packages(package_status_t *packages, int max_packages, int *count) {
    if (!packages || !count || max_packages <= 0) return -1;
    
    *count = 0;
    
    // Check npm outdated
    FILE *fp = popen("npm outdated --json 2>/dev/null | grep '\"' | head -10", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp) && *count < max_packages) {
            // Simple parsing - would need proper JSON parser for production
            package_status_t *pkg = &packages[*count];
            snprintf(pkg->manager, sizeof(pkg->manager), "npm");
            snprintf(pkg->status, sizeof(pkg->status), "outdated");
            (*count)++;
        }
        pclose(fp);
    }
    
    // Check pip outdated
    fp = popen("pip list --outdated 2>/dev/null | tail -n +3 | head -10", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp) && *count < max_packages) {
            package_status_t *pkg = &packages[*count];
            char name[64], current[32], latest[32];
            if (sscanf(line, "%63s %31s %31s", name, current, latest) >= 3) {
                snprintf(pkg->name, sizeof(pkg->name), "%s", name);
                snprintf(pkg->current_version, sizeof(pkg->current_version), "%s", current);
                snprintf(pkg->latest_version, sizeof(pkg->latest_version), "%s", latest);
                snprintf(pkg->manager, sizeof(pkg->manager), "pip");
                snprintf(pkg->status, sizeof(pkg->status), "outdated");
                (*count)++;
            }
        }
        pclose(fp);
    }
    
    // Check Homebrew outdated
    fp = popen("brew outdated 2>/dev/null | head -10", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp) && *count < max_packages) {
            package_status_t *pkg = &packages[*count];
            char name[128];
            if (sscanf(line, "%127s", name) >= 1) {
                snprintf(pkg->name, sizeof(pkg->name), "%s", name);
                snprintf(pkg->manager, sizeof(pkg->manager), "brew");
                snprintf(pkg->status, sizeof(pkg->status), "outdated");
                (*count)++;
            }
        }
        pclose(fp);
    }
    
    return 0;
}

// Execute developer quick actions
int execute_dev_action(dev_action_t action, char *output, size_t output_size) {
    if (!output || output_size == 0) return -1;
    
    const char *cmd = NULL;
    
    switch(action) {
        case DEV_ACTION_NPM_INSTALL:
            cmd = "npm install && echo 'Dependencies installed'";
            break;
        case DEV_ACTION_NPM_UPDATE:
            cmd = "npm update && echo 'Dependencies updated'";
            break;
        case DEV_ACTION_NPM_AUDIT:
            cmd = "npm audit fix && echo 'Security issues fixed'";
            break;
        case DEV_ACTION_CLEAR_NODE_MODULES:
            cmd = "rm -rf node_modules package-lock.json && echo 'node_modules cleared'";
            break;
        case DEV_ACTION_GIT_STATUS_ALL:
            cmd = "find ~ -type d -name '.git' -maxdepth 4 2>/dev/null | while read dir; do repo=$(dirname \"$dir\"); echo \"$repo:\"; cd \"$repo\" && git status -s; echo; done | head -50";
            break;
        case DEV_ACTION_KILL_PORT_3000:
            cmd = "lsof -ti:3000 | xargs kill -9 2>/dev/null && echo 'Port 3000 freed' || echo 'Port 3000 not in use'";
            break;
        case DEV_ACTION_KILL_ALL_NODE:
            cmd = "killall node 2>/dev/null && echo 'All Node processes killed' || echo 'No Node processes found'";
            break;
        case DEV_ACTION_START_POSTGRES:
            cmd = "pg_ctl start 2>/dev/null || brew services start postgresql 2>/dev/null && echo 'PostgreSQL started'";
            break;
        case DEV_ACTION_STOP_POSTGRES:
            cmd = "pg_ctl stop 2>/dev/null || brew services stop postgresql 2>/dev/null && echo 'PostgreSQL stopped'";
            break;
        case DEV_ACTION_START_REDIS:
            cmd = "redis-server --daemonize yes && echo 'Redis started'";
            break;
        case DEV_ACTION_STOP_REDIS:
            cmd = "redis-cli shutdown && echo 'Redis stopped'";
            break;
        case DEV_ACTION_DOCKER_PRUNE:
            cmd = "docker system prune -f && echo 'Docker cleaned'";
            break;
        case DEV_ACTION_BREW_UPDATE:
            cmd = "brew update && brew upgrade && echo 'Homebrew updated'";
            break;
        case DEV_ACTION_CLEAN_DERIVED_DATA:
            cmd = "rm -rf ~/Library/Developer/Xcode/DerivedData/* && echo 'DerivedData cleaned'";
            break;
        case DEV_ACTION_CLEAN_GRADLE_CACHE:
            cmd = "rm -rf ~/.gradle/caches/* && echo 'Gradle cache cleaned'";
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

// Get localhost servers status
int get_localhost_servers(localhost_server_t *servers, int max_servers, int *count) {
    if (!servers || !count || max_servers <= 0) return -1;
    
    *count = 0;
    
    // Common development ports to check
    int common_ports[] = {
        3000, 3001, 3002,  // React, Next.js
        4200,              // Angular
        5000, 5001,        // Flask, generic
        8000, 8001,        // Django, generic
        8080, 8081,        // Generic web
        9000,              // PHP
        4000,              // Phoenix
        1313,              // Hugo
        4567,              // Sinatra
        9292,              // Rack
        0
    };
    
    for (int i = 0; common_ports[i] && *count < max_servers; i++) {
        int port = common_ports[i];
        
        // Check if port is open
        char cmd[256];
        snprintf(cmd, sizeof(cmd), 
                "lsof -i:%d -P -n 2>/dev/null | grep LISTEN | head -1", port);
        
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char line[256];
            if (fgets(line, sizeof(line), fp)) {
                localhost_server_t *server = &servers[*count];
                server->port = port;
                server->is_running = 1;
                
                // Extract process name
                char process[64];
                int pid;
                if (sscanf(line, "%63s %d", process, &pid) >= 2) {
                    snprintf(server->process, sizeof(server->process), "%s", process);
                    server->pid = pid;
                }
                
                // Try to get response time
                snprintf(cmd, sizeof(cmd), 
                        "curl -o /dev/null -s -w '%%{time_total}' http://localhost:%d/ 2>/dev/null", 
                        port);
                FILE *curl_fp = popen(cmd, "r");
                if (curl_fp) {
                    fscanf(curl_fp, "%f", &server->response_time_ms);
                    server->response_time_ms *= 1000; // Convert to ms
                    pclose(curl_fp);
                }
                
                // Determine framework/type based on port and process
                if (strstr(process, "node")) {
                    snprintf(server->framework, sizeof(server->framework), "Node.js");
                } else if (strstr(process, "python")) {
                    snprintf(server->framework, sizeof(server->framework), 
                            port == 8000 ? "Django" : "Python");
                } else if (strstr(process, "ruby")) {
                    snprintf(server->framework, sizeof(server->framework), "Ruby");
                } else if (strstr(process, "java")) {
                    snprintf(server->framework, sizeof(server->framework), "Java");
                } else {
                    snprintf(server->framework, sizeof(server->framework), "Unknown");
                }
                
                (*count)++;
            }
            pclose(fp);
        }
    }
    
    return 0;
}

// Check security vulnerabilities
int check_security_issues(security_issue_t *issues, int max_issues, int *count) {
    if (!issues || !count || max_issues <= 0) return -1;
    
    *count = 0;
    
    // npm audit
    FILE *fp = popen("npm audit --json 2>/dev/null | grep -c '\"severity\"' | head -1", "r");
    if (fp) {
        int npm_issues = 0;
        if (fscanf(fp, "%d", &npm_issues) == 1 && npm_issues > 0 && *count < max_issues) {
            security_issue_t *issue = &issues[*count];
            snprintf(issue->type, sizeof(issue->type), "npm");
            snprintf(issue->description, sizeof(issue->description), 
                    "%d npm vulnerabilities found", npm_issues);
            issue->severity = npm_issues > 10 ? SECURITY_HIGH : 
                             npm_issues > 5 ? SECURITY_MEDIUM : SECURITY_LOW;
            snprintf(issue->fix_command, sizeof(issue->fix_command), "npm audit fix");
            (*count)++;
        }
        pclose(fp);
    }
    
    // Check for exposed .env files
    fp = popen("find . -name '.env' -maxdepth 3 2>/dev/null | head -5", "r");
    if (fp) {
        char line[256];
        int env_count = 0;