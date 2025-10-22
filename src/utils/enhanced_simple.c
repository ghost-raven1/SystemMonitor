// enhanced_simple.c - Simplified enhanced features for System Monitor
// This version works without external dependencies like curl or json-c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

// Enhanced weather function using wttr.in
void fetch_enhanced_weather_simple(char *output, size_t size) {
    if (!output || size == 0) return;
    
    // Get city from environment or use default
    const char *city = getenv("SYSMON_WEATHER_CITY");
    if (!city || !city[0]) {
        // Try to auto-detect city
        FILE *fp = popen("curl -s -m 2 'https://ipinfo.io/city' 2>/dev/null", "r");
        if (fp) {
            static char detected_city[64];
            if (fgets(detected_city, sizeof(detected_city), fp)) {
                detected_city[strcspn(detected_city, "\n")] = 0;
                city = detected_city;
            }
            pclose(fp);
        }
    }
    if (!city || !city[0]) city = "Moscow";
    
    // Get weather with enhanced format
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
            "curl -s -m 3 'https://wttr.in/%s?format=%%l:+%%t+(%%f)+%%C+%%h+%%w' 2>/dev/null", 
            city);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        snprintf(output, size, "🌡️ Нет данных");
        return;
    }
    
    char buf[256];
    if (fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\n")] = 0;
        
        // Parse and format the weather data
        char location[64], temp[32], feels[32], condition[64], humidity[32], wind[32];
        if (sscanf(buf, "%63[^:]: %31s (%31[^)]) %63s %31s %31s", 
                   location, temp, feels, condition, humidity, wind) >= 3) {
            
            // Determine weather icon based on condition
            const char *icon = "🌡️";
            char cond_lower[64];
            for (int i = 0; condition[i] && i < 63; i++) {
                cond_lower[i] = tolower(condition[i]);
            }
            cond_lower[63] = '\0';
            
            if (strstr(cond_lower, "clear") || strstr(cond_lower, "sunny")) icon = "☀️";
            else if (strstr(cond_lower, "cloud")) icon = "☁️";
            else if (strstr(cond_lower, "rain")) icon = "🌧️";
            else if (strstr(cond_lower, "snow")) icon = "❄️";
            else if (strstr(cond_lower, "storm")) icon = "⛈️";
            else if (strstr(cond_lower, "fog") || strstr(cond_lower, "mist")) icon = "🌫️";
            
            snprintf(output, size, "%s %s %s (ощущ. %s) %s %s", 
                    icon, location, temp, feels, humidity, wind);
        } else {
            snprintf(output, size, "%s", buf);
        }
    } else {
        snprintf(output, size, "🌡️ Нет данных");
    }
    
    pclose(fp);
}

// Get active network connections count
int get_network_stats_simple(int *tcp_count, int *udp_count, int *established) {
    *tcp_count = 0;
    *udp_count = 0;
    *established = 0;
    
    FILE *fp = popen("netstat -an 2>/dev/null | grep -E '^(tcp|udp)' | wc -l", "r");
    if (fp) {
        int total;
        if (fscanf(fp, "%d", &total) == 1) {
            *tcp_count = total;
        }
        pclose(fp);
    }
    
    fp = popen("netstat -an 2>/dev/null | grep ESTABLISHED | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", established);
        pclose(fp);
    }
    
    return 0;
}

// Get top network connections
void get_top_connections_simple(char *output, size_t size) {
    if (!output || size == 0) return;
    
    FILE *fp = popen("netstat -an 2>/dev/null | grep ESTABLISHED | head -5", "r");
    if (!fp) {
        snprintf(output, size, "Нет активных соединений");
        return;
    }
    
    char line[256];
    int count = 0;
    size_t pos = 0;
    
    while (fgets(line, sizeof(line), fp) && count < 5) {
        char proto[16], local[64], remote[64];
        if (sscanf(line, "%15s %*s %*s %63s %63s", proto, local, remote) >= 3) {
            // Format connection info
            char conn_info[128];
            
            // Try to identify service by port
            const char *service = "";
            if (strstr(remote, ".443")) service = " (HTTPS)";
            else if (strstr(remote, ".80")) service = " (HTTP)";
            else if (strstr(remote, ".22")) service = " (SSH)";
            else if (strstr(remote, ".21")) service = " (FTP)";
            else if (strstr(remote, ".25")) service = " (SMTP)";
            else if (strstr(remote, ".3306")) service = " (MySQL)";
            else if (strstr(remote, ".5432")) service = " (PostgreSQL)";
            
            snprintf(conn_info, sizeof(conn_info), "%s → %s%s\n", 
                    local, remote, service);
            
            size_t len = strlen(conn_info);
            if (pos + len < size - 1) {
                strcat(output + pos, conn_info);
                pos += len;
            }
            count++;
        }
    }
    
    pclose(fp);
    
    if (count == 0) {
        snprintf(output, size, "Нет активных соединений");
    }
}

// Check if Docker is available
int check_docker_simple() {
    return system("which docker >/dev/null 2>&1") == 0;
}

// Get Docker container count
int get_docker_stats_simple(int *running, int *total) {
    *running = 0;
    *total = 0;
    
    if (!check_docker_simple()) return -1;
    
    FILE *fp = popen("docker ps -q 2>/dev/null | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", running);
        pclose(fp);
    }
    
    fp = popen("docker ps -aq 2>/dev/null | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", total);
        pclose(fp);
    }
    
    return 0;
}

// Get system events from log
void get_recent_events_simple(char *output, size_t size) {
    if (!output || size == 0) return;
    
    // Try to get recent system events
    FILE *fp = popen("log show --style syslog --last 1m 2>/dev/null | grep -E '(error|warning|fail)' -i | tail -5", "r");
    if (!fp) {
        // Fallback to system.log
        fp = popen("tail -10 /var/log/system.log 2>/dev/null | grep -E '(error|warning|fail)' -i | tail -5", "r");
    }
    
    if (!fp) {
        snprintf(output, size, "Нет событий");
        return;
    }
    
    char line[256];
    int count = 0;
    size_t pos = 0;
    
    while (fgets(line, sizeof(line), fp) && count < 5) {
        // Simplify and format the event
        char *start = line;
        // Skip timestamp if present
        char *msg = strstr(line, ": ");
        if (msg) start = msg + 2;
        
        // Determine event type
        const char *icon = "ℹ️";
        char lower[256];
        for (int i = 0; start[i] && i < 255; i++) {
            lower[i] = tolower(start[i]);
        }
        lower[255] = '\0';
        
        if (strstr(lower, "error") || strstr(lower, "fail")) icon = "❌";
        else if (strstr(lower, "warning") || strstr(lower, "warn")) icon = "⚠️";
        
        // Truncate long messages
        if (strlen(start) > 60) {
            start[60] = '\0';
            strcat(start, "...");
        }
        
        char event_line[128];
        snprintf(event_line, sizeof(event_line), "%s %s\n", icon, start);
        
        size_t len = strlen(event_line);
        if (pos + len < size - 1) {
            strcat(output + pos, event_line);
            pos += len;
        }
        count++;
    }
    
    pclose(fp);
    
    if (count == 0) {
        snprintf(output, size, "Нет недавних событий");
    }
}

// Execute quick system action
int execute_quick_action_simple(int action_id, char *result, size_t size) {
    if (!result || size == 0) return -1;
    
    const char *cmd = NULL;
    
    switch(action_id) {
        case 0: // Clean cache
            cmd = "rm -rf ~/Library/Caches/* 2>/dev/null && echo 'Кэш очищен'";
            break;
        case 1: // Restart network
            cmd = "sudo ifconfig en0 down && sudo ifconfig en0 up && echo 'Сеть перезапущена'";
            break;
        case 2: // Flush DNS
            cmd = "sudo dscacheutil -flushcache && echo 'DNS кэш очищен'";
            break;
        case 3: // Free memory
            cmd = "sudo purge && echo 'Память освобождена'";
            break;
        case 4: // Check updates
            cmd = "softwareupdate -l 2>&1 | head -5";
            break;
        case 5: // Show large files
            cmd = "find ~ -type f -size +100M 2>/dev/null | head -10";
            break;
        case 6: // Restart Dock
            cmd = "killall Dock && echo 'Dock перезапущен'";
            break;
        case 7: // Restart Finder
            cmd = "killall Finder && echo 'Finder перезапущен'";
            break;
        case 8: // Toggle hidden files
            cmd = "defaults read com.apple.finder AppleShowAllFiles | grep -q YES && "
                 "defaults write com.apple.finder AppleShowAllFiles NO || "
                 "defaults write com.apple.finder AppleShowAllFiles YES; "
                 "killall Finder; echo 'Видимость скрытых файлов изменена'";
            break;
        case 9: // Empty trash
            cmd = "rm -rf ~/.Trash/* 2>/dev/null && echo 'Корзина очищена'";
            break;
        default:
            snprintf(result, size, "Неизвестное действие");
            return -1;
    }
    
    if (cmd) {
        FILE *fp = popen(cmd, "r");
        if (fp) {
            size_t n = fread(result, 1, size - 1, fp);
            result[n] = '\0';
            pclose(fp);
            return 0;
        }
    }
    
    snprintf(result, size, "Ошибка выполнения");
    return -1;
}

// Get system health score (0-100)
int get_system_health_score() {
    int score = 100;
    
    // Check CPU usage
    FILE *fp = popen("ps -A -o %cpu | awk '{s+=$1} END {print s}'", "r");
    if (fp) {
        float cpu_total;
        if (fscanf(fp, "%f", &cpu_total) == 1) {
            if (cpu_total > 200) score -= 20;
            else if (cpu_total > 100) score -= 10;
        }
        pclose(fp);
    }
    
    // Check memory pressure
    fp = popen("vm_stat | grep 'Pages free' | awk '{print $3}' | sed 's/\\.//'", "r");
    if (fp) {
        long pages_free;
        if (fscanf(fp, "%ld", &pages_free) == 1) {
            if (pages_free < 5000) score -= 30;
            else if (pages_free < 10000) score -= 15;
        }
        pclose(fp);
    }
    
    // Check disk space
    fp = popen("df -h / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
    if (fp) {
        int disk_usage;
        if (fscanf(fp, "%d", &disk_usage) == 1) {
            if (disk_usage > 90) score -= 25;
            else if (disk_usage > 80) score -= 10;
        }
        pclose(fp);
    }
    
    // Check swap usage
    fp = popen("sysctl vm.swapusage | awk '{print $7}' | sed 's/M//'", "r");
    if (fp) {
        float swap_used;
        if (fscanf(fp, "%f", &swap_used) == 1) {
            if (swap_used > 1000) score -= 15;
            else if (swap_used > 500) score -= 5;
        }
        pclose(fp);
    }
    
    return score > 0 ? score : 0;
}

// Get system recommendations
void get_system_recommendations_simple(char *output, size_t size) {
    if (!output || size == 0) return;
    
    output[0] = '\0';
    size_t pos = 0;
    
    // Check disk space
    FILE *fp = popen("df -h / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
    if (fp) {
        int disk_usage;
        if (fscanf(fp, "%d", &disk_usage) == 1 && disk_usage > 80) {
            char rec[128];
            snprintf(rec, sizeof(rec), "🔴 Мало места на диске (%d%%). Очистите кэш.\n", disk_usage);
            if (pos + strlen(rec) < size - 1) {
                strcat(output + pos, rec);
                pos += strlen(rec);
            }
        }
        pclose(fp);
    }
    
    // Check memory
    fp = popen("vm_stat | grep 'Pages free' | awk '{print $3}' | sed 's/\\.//'", "r");
    if (fp) {
        long pages_free;
        if (fscanf(fp, "%ld", &pages_free) == 1 && pages_free < 10000) {
            char rec[128];
            snprintf(rec, sizeof(rec), "🟡 Мало свободной памяти. Закройте приложения.\n");
            if (pos + strlen(rec) < size - 1) {
                strcat(output + pos, rec);
                pos += strlen(rec);
            }
        }
        pclose(fp);
    }
    
    // Check for updates
    fp = popen("softwareupdate -l 2>&1 | grep -c 'Software Update found'", "r");
    if (fp) {
        int updates;
        if (fscanf(fp, "%d", &updates) == 1 && updates > 0) {
            char rec[128];
            snprintf(rec, sizeof(rec), "🔵 Доступны обновления системы.\n");
            if (pos + strlen(rec) < size - 1) {
                strcat(output + pos, rec);
                pos += strlen(rec);
            }
        }
        pclose(fp);
    }
    
    if (pos == 0) {
        snprintf(output, size, "✅ Система работает оптимально");
    }
}

// Get top processes by CPU
void get_top_processes_simple(char *output, size_t size) {
    if (!output || size == 0) return;
    
    FILE *fp = popen("ps aux | sort -rk 3 | head -6 | tail -5", "r");
    if (!fp) {
        snprintf(output, size, "Нет данных");
        return;
    }
    
    char line[256];
    size_t pos = 0;
    output[0] = '\0';
    
    // Skip header
    if (fgets(line, sizeof(line), fp)) {
        while (fgets(line, sizeof(line), fp)) {
            char user[32], cmd[128];
            float cpu, mem;
            int pid;
            
            if (sscanf(line, "%31s %d %f %f %*s %*s %*s %*s %*s %*s %127[^\n]", 
                       user, &pid, &cpu, &mem, cmd) >= 5) {
                
                // Simplify command name
                char *cmd_name = strrchr(cmd, '/');
                if (cmd_name) cmd_name++;
                else cmd_name = cmd;
                
                // Truncate long names
                if (strlen(cmd_name) > 20) {
                    cmd_name[20] = '\0';
                    strcat(cmd_name, "...");
                }
                
                char proc_line[128];
                snprintf(proc_line, sizeof(proc_line), "%-20s %5.1f%% %5.1f%%\n", 
                        cmd_name, cpu, mem);
                
                if (pos + strlen(proc_line) < size - 1) {
                    strcat(output + pos, proc_line);
                    pos += strlen(proc_line);
                }
            }
        }
    }
    
    pclose(fp);
    
    if (pos == 0) {
        snprintf(output, size, "Нет данных");
    }
}