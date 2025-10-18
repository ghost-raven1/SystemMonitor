// enhanced.c - Enhanced features for System Monitor
// Weather, system events, network connections, and more

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysctl.h>
#include <libproc.h>
#include <errno.h>
#include <signal.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "enhanced.h"
#include "logging.h"

// Weather icon mappings
static const weather_icon_t weather_icons[] = {
    {"clear", "☀️", "Ясно"},
    {"sunny", "☀️", "Солнечно"},
    {"partly cloudy", "⛅", "Переменная облачность"},
    {"cloudy", "☁️", "Облачно"},
    {"overcast", "☁️", "Пасмурно"},
    {"rain", "🌧️", "Дождь"},
    {"light rain", "🌦️", "Небольшой дождь"},
    {"heavy rain", "⛈️", "Сильный дождь"},
    {"drizzle", "🌦️", "Морось"},
    {"snow", "❄️", "Снег"},
    {"light snow", "🌨️", "Небольшой снег"},
    {"heavy snow", "❄️", "Снегопад"},
    {"sleet", "🌨️", "Мокрый снег"},
    {"fog", "🌫️", "Туман"},
    {"mist", "🌫️", "Дымка"},
    {"thunderstorm", "⛈️", "Гроза"},
    {"tornado", "🌪️", "Торнадо"},
    {"hurricane", "🌀", "Ураган"},
    {NULL, NULL, NULL}
};

// Cache structures
static weather_cache_t weather_cache = {0};
static network_connections_cache_t net_conn_cache = {0};
static system_events_cache_t sys_events_cache = {0};
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// CURL write callback
static size_t curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    weather_response_t *resp = (weather_response_t *)userp;
    
    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if (!ptr) return 0;
    
    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;
    
    return realsize;
}

// Get weather icon based on condition
static const char* get_weather_icon(const char *condition) {
    if (!condition) return "❓";
    
    char lower[256];
    size_t len = strlen(condition);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    
    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower(condition[i]);
    }
    lower[len] = '\0';
    
    for (int i = 0; weather_icons[i].condition; i++) {
        if (strstr(lower, weather_icons[i].condition)) {
            return weather_icons[i].icon;
        }
    }
    
    return "🌡️";
}

// Fetch enhanced weather data
int fetch_enhanced_weather(enhanced_weather_t *weather) {
    if (!weather) return -1;
    
    pthread_mutex_lock(&cache_mutex);
    
    time_t now = time(NULL);
    // Check cache (5 minutes)
    if (weather_cache.valid && (now - weather_cache.timestamp) < 300) {
        memcpy(weather, &weather_cache.data, sizeof(enhanced_weather_t));
        pthread_mutex_unlock(&cache_mutex);
        return 0;
    }
    
    // Get location
    char location[128] = {0};
    const char *env_city = getenv("SYSMON_WEATHER_CITY");
    const char *api_key = getenv("SYSMON_WEATHER_API_KEY");
    
    if (env_city && env_city[0]) {
        snprintf(location, sizeof(location), "%s", env_city);
    } else {
        // Auto-detect location
        FILE *fp = popen("curl -s -m 2 'https://ipinfo.io/city'", "r");
        if (fp) {
            fgets(location, sizeof(location), fp);
            pclose(fp);
            // Remove newline
            location[strcspn(location, "\n")] = 0;
        }
    }
    
    if (!location[0]) {
        strcpy(location, "Moscow");
    }
    
    // Prepare weather data
    memset(weather, 0, sizeof(enhanced_weather_t));
    snprintf(weather->city, sizeof(weather->city), "%s", location);
    
    if (api_key && api_key[0]) {
        // Use OpenWeatherMap API for detailed data
        char url[512];
        snprintf(url, sizeof(url), 
                "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric&lang=ru",
                location, api_key);
        
        CURL *curl = curl_easy_init();
        if (curl) {
            weather_response_t resp = {0};
            
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res == CURLE_OK && resp.data) {
                // Parse JSON response
                struct json_object *root = json_tokener_parse(resp.data);
                if (root) {
                    struct json_object *main_obj, *weather_arr, *weather_obj, *wind_obj, *sys_obj;
                    
                    if (json_object_object_get_ex(root, "main", &main_obj)) {
                        struct json_object *temp, *feels, *humid, *press;
                        if (json_object_object_get_ex(main_obj, "temp", &temp))
                            weather->temperature = json_object_get_double(temp);
                        if (json_object_object_get_ex(main_obj, "feels_like", &feels))
                            weather->feels_like = json_object_get_double(feels);
                        if (json_object_object_get_ex(main_obj, "humidity", &humid))
                            weather->humidity = json_object_get_int(humid);
                        if (json_object_object_get_ex(main_obj, "pressure", &press))
                            weather->pressure = json_object_get_int(press);
                    }
                    
                    if (json_object_object_get_ex(root, "weather", &weather_arr)) {
                        if (json_object_array_length(weather_arr) > 0) {
                            weather_obj = json_object_array_get_idx(weather_arr, 0);
                            struct json_object *desc;
                            if (json_object_object_get_ex(weather_obj, "description", &desc)) {
                                snprintf(weather->condition, sizeof(weather->condition), 
                                        "%s", json_object_get_string(desc));
                            }
                        }
                    }
                    
                    if (json_object_object_get_ex(root, "wind", &wind_obj)) {
                        struct json_object *speed, *deg;
                        if (json_object_object_get_ex(wind_obj, "speed", &speed))
                            weather->wind_speed = json_object_get_double(speed);
                        if (json_object_object_get_ex(wind_obj, "deg", &deg))
                            weather->wind_dir = json_object_get_int(deg);
                    }
                    
                    if (json_object_object_get_ex(root, "sys", &sys_obj)) {
                        struct json_object *sunrise, *sunset;
                        if (json_object_object_get_ex(sys_obj, "sunrise", &sunrise))
                            weather->sunrise = json_object_get_int(sunrise);
                        if (json_object_object_get_ex(sys_obj, "sunset", &sunset))
                            weather->sunset = json_object_get_int(sunset);
                    }
                    
                    json_object_put(root);
                    weather->valid = 1;
                }
            }
            
            if (resp.data) free(resp.data);
            curl_easy_cleanup(curl);
        }
    } else {
        // Fallback to wttr.in
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "curl -s -m 3 'https://wttr.in/%s?format=%%t|%%C|%%h|%%w|%%p'", location);
        
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char buf[256];
            if (fgets(buf, sizeof(buf), fp)) {
                char temp_str[32], cond[64], humid_str[32], wind[32], press[32];
                if (sscanf(buf, "%31[^|]|%63[^|]|%31[^|]|%31[^|]|%31s", 
                          temp_str, cond, humid_str, wind, press) >= 2) {
                    
                    // Parse temperature
                    sscanf(temp_str, "%f", &weather->temperature);
                    
                    // Parse condition
                    snprintf(weather->condition, sizeof(weather->condition), "%s", cond);
                    
                    // Parse humidity
                    sscanf(humid_str, "%d%%", &weather->humidity);
                    
                    // Parse wind
                    sscanf(wind, "%fkm/h", &weather->wind_speed);
                    weather->wind_speed /= 3.6; // Convert to m/s
                    
                    // Parse pressure
                    sscanf(press, "%dhPa", &weather->pressure);
                    
                    weather->valid = 1;
                }
            }
            pclose(fp);
        }
    }
    
    // Set icon based on condition
    if (weather->valid) {
        snprintf(weather->icon, sizeof(weather->icon), "%s", 
                get_weather_icon(weather->condition));
        
        // Cache the result
        memcpy(&weather_cache.data, weather, sizeof(enhanced_weather_t));
        weather_cache.timestamp = now;
        weather_cache.valid = 1;
    }
    
    pthread_mutex_unlock(&cache_mutex);
    return weather->valid ? 0 : -1;
}

// Get network connections
int get_network_connections(network_connection_t *conns, int max_conns, int *count) {
    if (!conns || !count || max_conns <= 0) return -1;
    
    *count = 0;
    
    // Get TCP connections using netstat
    FILE *fp = popen("netstat -an -p tcp | grep ESTABLISHED | head -50", "r");
    if (!fp) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), fp) && *count < max_conns) {
        network_connection_t *conn = &conns[*count];
        char proto[16], local[128], remote[128], state[32];
        
        if (sscanf(line, "%15s %*d %*d %127s %127s %31s", 
                   proto, local, remote, state) >= 4) {
            
            snprintf(conn->protocol, sizeof(conn->protocol), "%s", proto);
            snprintf(conn->local_addr, sizeof(conn->local_addr), "%s", local);
            snprintf(conn->remote_addr, sizeof(conn->remote_addr), "%s", remote);
            snprintf(conn->state, sizeof(conn->state), "%s", state);
            
            // Try to resolve service name
            char *port_str = strrchr(remote, '.');
            if (port_str && port_str[1]) {
                int port = atoi(port_str + 1);
                switch(port) {
                    case 80: snprintf(conn->service, sizeof(conn->service), "HTTP"); break;
                    case 443: snprintf(conn->service, sizeof(conn->service), "HTTPS"); break;
                    case 22: snprintf(conn->service, sizeof(conn->service), "SSH"); break;
                    case 21: snprintf(conn->service, sizeof(conn->service), "FTP"); break;
                    case 25: snprintf(conn->service, sizeof(conn->service), "SMTP"); break;
                    case 110: snprintf(conn->service, sizeof(conn->service), "POP3"); break;
                    case 143: snprintf(conn->service, sizeof(conn->service), "IMAP"); break;
                    case 3306: snprintf(conn->service, sizeof(conn->service), "MySQL"); break;
                    case 5432: snprintf(conn->service, sizeof(conn->service), "PostgreSQL"); break;
                    case 6379: snprintf(conn->service, sizeof(conn->service), "Redis"); break;
                    case 27017: snprintf(conn->service, sizeof(conn->service), "MongoDB"); break;
                    default: snprintf(conn->service, sizeof(conn->service), "Port %d", port); break;
                }
            }
            
            (*count)++;
        }
    }
    
    pclose(fp);
    return 0;
}

// Get system events
int get_system_events(system_event_t *events, int max_events, int *count) {
    if (!events || !count || max_events <= 0) return -1;
    
    *count = 0;
    
    // Get recent system events from system.log
    FILE *fp = popen("log show --style syslog --predicate 'eventMessage contains[c] \"error\" OR eventMessage contains[c] \"warning\" OR eventMessage contains[c] \"fail\"' --last 1m 2>/dev/null | tail -20", "r");
    if (!fp) {
        // Fallback to /var/log/system.log
        fp = popen("tail -20 /var/log/system.log 2>/dev/null | grep -E '(error|warning|fail|crash)' -i", "r");
    }
    
    if (!fp) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), fp) && *count < max_events) {
        system_event_t *event = &events[*count];
        
        // Parse timestamp and message
        struct tm tm;
        char month[4], process[64], message[256];
        int day, hour, min, sec;
        
        // Try to parse syslog format
        if (sscanf(line, "%3s %d %d:%d:%d %*s %63[^:]: %255[^\n]",
                   month, &day, &hour, &min, &sec, process, message) >= 7) {
            
            event->timestamp = time(NULL); // Use current time as approximation
            snprintf(event->process, sizeof(event->process), "%s", process);
            snprintf(event->message, sizeof(event->message), "%s", message);
            
            // Determine severity
            char lower[256];
            size_t len = strlen(message);
            if (len >= sizeof(lower)) len = sizeof(lower) - 1;
            for (size_t i = 0; i < len; i++) {
                lower[i] = tolower(message[i]);
            }
            lower[len] = '\0';
            
            if (strstr(lower, "error") || strstr(lower, "fail") || strstr(lower, "crash")) {
                event->severity = EVENT_ERROR;
                snprintf(event->icon, sizeof(event->icon), "🔴");
            } else if (strstr(lower, "warning") || strstr(lower, "warn")) {
                event->severity = EVENT_WARNING;
                snprintf(event->icon, sizeof(event->icon), "🟡");
            } else {
                event->severity = EVENT_INFO;
                snprintf(event->icon, sizeof(event->icon), "🔵");
            }
            
            (*count)++;
        }
    }
    
    pclose(fp);
    return 0;
}

// Check Docker status
int check_docker_status(docker_status_t *status) {
    if (!status) return -1;
    
    memset(status, 0, sizeof(docker_status_t));
    
    // Check if Docker is running
    FILE *fp = popen("docker version --format '{{.Server.Version}}' 2>/dev/null", "r");
    if (!fp) {
        status->running = 0;
        return -1;
    }
    
    char version[64];
    if (fgets(version, sizeof(version), fp)) {
        status->running = 1;
        version[strcspn(version, "\n")] = 0;
        snprintf(status->version, sizeof(status->version), "%s", version);
    }
    pclose(fp);
    
    if (!status->running) return -1;
    
    // Get container count
    fp = popen("docker ps -q | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &status->containers_running);
        pclose(fp);
    }
    
    fp = popen("docker ps -aq | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &status->containers_total);
        pclose(fp);
    }
    
    // Get image count
    fp = popen("docker images -q | sort -u | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &status->images);
        pclose(fp);
    }
    
    // Get volume count
    fp = popen("docker volume ls -q | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &status->volumes);
        pclose(fp);
    }
    
    // Get network count
    fp = popen("docker network ls -q | wc -l", "r");
    if (fp) {
        fscanf(fp, "%d", &status->networks);
        pclose(fp);
    }
    
    return 0;
}

// Get container list
int get_docker_containers(docker_container_t *containers, int max_containers, int *count) {
    if (!containers || !count || max_containers <= 0) return -1;
    
    *count = 0;
    
    FILE *fp = popen("docker ps --format '{{.ID}}|{{.Names}}|{{.Image}}|{{.Status}}|{{.Ports}}'", "r");
    if (!fp) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), fp) && *count < max_containers) {
        docker_container_t *cont = &containers[*count];
        char id[16], name[64], image[128], status[64], ports[128];
        
        if (sscanf(line, "%15[^|]|%63[^|]|%127[^|]|%63[^|]|%127[^\n]",
                   id, name, image, status, ports) >= 4) {
            
            snprintf(cont->id, sizeof(cont->id), "%s", id);
            snprintf(cont->name, sizeof(cont->name), "%s", name);
            snprintf(cont->image, sizeof(cont->image), "%s", image);
            snprintf(cont->status, sizeof(cont->status), "%s", status);
            snprintf(cont->ports, sizeof(cont->ports), "%s", ports);
            
            // Get CPU and memory usage
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "docker stats --no-stream --format '{{.CPUPerc}}|{{.MemUsage}}' %s 2>/dev/null", id);
            FILE *stats_fp = popen(cmd, "r");
            if (stats_fp) {
                char stats_line[128];
                if (fgets(stats_line, sizeof(stats_line), stats_fp)) {
                    char cpu_str[32], mem_str[64];
                    if (sscanf(stats_line, "%31[^|]|%63s", cpu_str, mem_str) == 2) {
                        sscanf(cpu_str, "%f%%", &cont->cpu_percent);
                        snprintf(cont->memory_usage, sizeof(cont->memory_usage), "%s", mem_str);
                    }
                }
                pclose(stats_fp);
            }
            
            // Determine if running
            cont->running = (strstr(status, "Up") != NULL);
            
            (*count)++;
        }
    }
    
    pclose(fp);
    return 0;
}

// Quick action executor
int execute_quick_action(quick_action_t action, char *output, size_t output_size) {
    if (!output || output_size == 0) return -1;
    
    const char *cmd = NULL;
    
    switch(action) {
        case ACTION_CLEAN_CACHE:
            cmd = "sudo rm -rf ~/Library/Caches/* /Library/Caches/* /System/Library/Caches/* 2>/dev/null; echo 'Кэш очищен'";
            break;
        case ACTION_RESTART_NETWORK:
            cmd = "sudo ifconfig en0 down && sudo ifconfig en0 up && echo 'Сеть перезапущена'";
            break;
        case ACTION_FLUSH_DNS:
            cmd = "sudo dscacheutil -flushcache && sudo killall -HUP mDNSResponder && echo 'DNS кэш очищен'";
            break;
        case ACTION_FREE_MEMORY:
            cmd = "sudo purge && echo 'Память освобождена'";
            break;
        case ACTION_UPDATE_SYSTEM:
            cmd = "softwareupdate -l 2>&1 | head -20";
            break;
        case ACTION_SHOW_LARGE_FILES:
            cmd = "find ~ -type f -size +100M 2>/dev/null | head -20";
            break;
        case ACTION_KILL_DOCK:
            cmd = "killall Dock && echo 'Dock перезапущен'";
            break;
        case ACTION_KILL_FINDER:
            cmd = "killall Finder && echo 'Finder перезапущен'";
            break;
        case ACTION_TOGGLE_HIDDEN_FILES:
            cmd = "defaults read com.apple.finder AppleShowAllFiles | grep -q YES && defaults write com.apple.finder AppleShowAllFiles NO || defaults write com.apple.finder AppleShowAllFiles YES; killall Finder; echo 'Видимость скрытых файлов переключена'";
            break;
        case ACTION_EMPTY_TRASH:
            cmd = "rm -rf ~/.Trash/* && echo 'Корзина очищена'";
            break;
        default:
            snprintf(output, output_size, "Неизвестное действие");
            return -1;
    }
    
    if (cmd) {
        FILE *fp = popen(cmd, "r");
        if (fp) {
            size_t n = fread(output, 1, output_size - 1, fp);
            output[n] = '\0';
            int ret = pclose(fp);
            return (ret == 0) ? 0 : -1;
        }
    }
    
    return -1;
}

// Get system recommendations
int get_system_recommendations(recommendation_t *recs, int max_recs, int *count) {
    if (!recs || !count || max_recs <= 0) return -1;
    
    *count = 0;
    
    // Check various system conditions and make recommendations
    
    // 1. Check disk space
    FILE *fp = popen("df -h / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
    if (fp) {
        int disk_usage;
        if (fscanf(fp, "%d", &disk_usage) == 1 && disk_usage > 80) {
            recommendation_t *rec = &recs[(*count)++];
            rec->severity = disk_usage > 90 ? REC_CRITICAL : REC_WARNING;
            snprintf(rec->icon, sizeof(rec->icon), disk_usage > 90 ? "🔴" : "🟡");
            snprintf(rec->title, sizeof(rec->title), "Мало свободного места на диске");
            snprintf(rec->description, sizeof(rec->description), 
                    "Использовано %d%% дискового пространства. Рекомендуется очистить кэш и удалить ненужные файлы.", 
                    disk_usage);
            rec->action = ACTION_SHOW_LARGE_FILES;
            if (*count >= max_recs) { pclose(fp); return 0; }
        }
        pclose(fp);
    }
    
    // 2. Check memory pressure
    fp = popen("vm_stat | grep 'Pages free' | awk '{print $3}' | sed 's/\\.//'", "r");
    if (fp) {
        long pages_free;
        if (fscanf(fp, "%ld", &pages_free) == 1 && pages_free < 10000) {
            recommendation_t *rec = &recs[(*count)++];
            rec->severity = pages_free < 5000 ? REC_CRITICAL : REC_WARNING;
            snprintf(rec->icon, sizeof(rec->icon), pages_free < 5000 ? "🔴" : "🟡");
            snprintf(rec->title, sizeof(rec->title), "Низкий уровень свободной памяти");
            snprintf(rec->description, sizeof(rec->description), 
                    "Свободно всего %ld страниц памяти. Рекомендуется закрыть неиспользуемые приложения.", 
                    pages_free);
            rec->action = ACTION_FREE_MEMORY;
            if (*count >= max_recs) { pclose(fp); return 0; }
        }
        pclose(fp);
    }
    
    // 3. Check for system updates
    fp = popen("softwareupdate -l 2>&1 | grep -c 'Software Update found'", "r");
    if (fp) {
        int updates;
        if (fscanf(fp, "%d", &updates) == 1 && updates > 0) {
            recommendation_t *rec = &recs[(*count)++];
            rec->severity = REC_INFO;
            snprintf(rec->icon, sizeof(rec->icon), "🔵");
            snprintf(rec->title, sizeof(rec->title), "Доступны обновления системы");
            snprintf(rec->description, sizeof(rec->description), 
                    "Найдено %d обновлений. Рекомендуется установить для повышения безопасности и стабильности.", 
                    updates);
            rec->action = ACTION_UPDATE_SYSTEM;
            if (*count >= max_recs) { pclose(fp); return 0; }
        }
        pclose(fp);
    }
    
    // 4. Check cache size
    fp = popen("du -sh ~/Library/Caches 2>/dev/null | awk '{print $1}'", "r");
    if (fp) {
        char size[32];
        if (fgets(size, sizeof(size), fp)) {
            size[strcspn(size, "\n")] = 0;
            // Check if size is in GB
            if (strstr(size, "G")) {
                recommendation_t *rec = &recs[(*count)++];
                rec->severity = REC_WARNING;
                snprintf(rec->icon, sizeof(rec->icon), "🟡");
                snprintf(rec->title, sizeof(rec->title), "Большой размер кэша");
                snprintf(rec->description, sizeof(rec->description), 
                        "Размер кэша: %s. Рекомендуется очистить для освобождения места.", size);
                rec->action = ACTION_CLEAN_CACHE;
                if (*count >= max_recs) { pclose(fp); return 0; }
            }
        }
        pclose(fp);
    }
    
    return 0;
}

// Format bytes to human readable
void format_bytes(unsigned long long bytes, char *output, size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double value = (double)bytes;
    
    while (value >= 1024 && unit_idx < 4) {
        value /= 1024;
        unit_idx++;
    }
    
    if (unit_idx == 0) {
        snprintf(output, size, "%llu %s", bytes, units[0]);
    } else {
        snprintf(output, size, "%.2f %s", value, units[unit_idx]);
    }
}

// Get wind direction as string
const char* get_wind_direction(int degrees) {
    if (degrees < 0) return "—";
    degrees = degrees % 360;
    
    const char *directions[] = {"С", "СВ", "В", "ЮВ", "Ю", "ЮЗ", "З", "СЗ"};
    int index = (degrees + 22) / 45;
    return directions[index % 8];
}

// Format time for display
void format_time(time_t timestamp, char *output, size_t size) {
    struct tm *tm_info = localtime(&timestamp);
    strftime(output, size, "%H:%M", tm_info);
}