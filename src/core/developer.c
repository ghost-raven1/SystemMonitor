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
#include "core/developer.h"


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
        while (fgets(line, sizeof(line), fp) && *count < max_issues) {
            security_issue_t *issue = &issues[*count];
            snprintf(issue->type, sizeof(issue->type), "env_file");
            snprintf(issue->description, sizeof(issue->description),
                    "Exposed .env file: %s", line);
            issue->severity = SECURITY_HIGH;
            snprintf(issue->fix_command, sizeof(issue->fix_command),
                    "chmod 600 %s", line);
            (*count)++;
        }
        pclose(fp);
    }

    return 0;
}