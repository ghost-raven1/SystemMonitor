#include "ui/ui_helpers.h"
#include "ui/ui.h"
#include <ncurses.h>
#include <sys/sysctl.h>
#include <stdarg.h>
#include <string.h>

// Helper function to get total memory in bytes
long get_total_memory(void) {
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    long total_mem;
    size_t len = sizeof(total_mem);
    if (sysctl(mib, 2, &total_mem, &len, NULL, 0) == 0) {
        return total_mem;
    }
    return 0;
}

// clipped print helper to keep lines within terminal width and avoid wrapping
void mvprintw_clip(int y, int x, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    int cols = 0;
    getmaxyx(stdscr, (int){0}, cols);
    int maxlen = cols - x - 1;
    if (maxlen <= 0) return;
    if ((int)strlen(buf) > maxlen) {
        // ensure we cut cleanly
        buf[maxlen] = '\0';
    }
    mvaddstr(y, x, buf);
}

// colored clipped print
void mvprintw_clip_color(int y, int x, int color_pair, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    int cols = 0;
    getmaxyx(stdscr, (int){0}, cols);
    int maxlen = cols - x - 1;
    if (maxlen <= 0) return;
    if ((int)strlen(buf) > maxlen) buf[maxlen] = '\0';
    attron(COLOR_PAIR(color_pair));
    mvaddstr(y, x, buf);
    attroff(COLOR_PAIR(color_pair));
}

// draw horizontal separator line across terminal width starting at x
void draw_hsep(int y, int x) {
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (y < 0 || y >= rows) return;
    int len = cols - x - 1;
    if (len <= 0) return;
    for (int i = 0; i < len; i++) mvaddch(y, x + i, '-');
}

// render a simple colored bar for percent [0..100]
void render_bar(int y, int x, int width, float percent, int colors_on) {
    if (width <= 0) return;
    if (percent < 0) percent = 0; if (percent > 100) percent = 100;
    int filled = (int)((percent / 100.0f) * width);
    int pair = 2; // green
    if (percent >= 90.0f) pair = 1; // red
    else if (percent >= 75.0f) pair = 3; // yellow
    if (colors_on) attron(COLOR_PAIR(pair));
    for (int i = 0; i < width; i++) mvaddch(y, x + i, i < filled ? '#' : ' ');
    if (colors_on) attroff(COLOR_PAIR(pair));
}

// pick color pair by percent thresholds (green/yellow/red)
int choose_color_by_percent(float percent, float warn, float crit) {
    if (percent >= crit) return 1;  // red
    if (percent >= warn) return 3;  // yellow
    return 2;                       // green
}

const char *resolve_config_path() {
    const char *cfg = getenv("SYSMON_CONFIG");
    if (cfg && cfg[0]) return cfg;
    return NULL; // caller will try defaults
}

// cache Wi‑Fi SSID for 60s (macOS)
void fetch_ssid_cached(char *out, size_t out_sz) {
    static char cache[64];
    static time_t cache_ts = 0;
    time_t now = time(NULL);
    if (cache[0] && (now - cache_ts) < 60) { snprintf(out, out_sz, "%s", cache); return; }
    FILE *fp = popen("/usr/sbin/networksetup -getairportnetwork en0 2>/dev/null | awk -F': ' '{print $2}'", "r");
    char buf[80]; size_t n = 0; buf[0] = '\0';
    if (fp) { n = fread(buf, 1, sizeof(buf)-1, fp); buf[n] = '\0'; pclose(fp); }
    for (size_t i = 0; i < n; i++) { if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = '\0'; break; } }
    if (!buf[0]) {
        fp = popen("/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I 2>/dev/null | awk -F': ' '/ SSID/ {print $2; exit}'", "r");
        if (fp) { n = fread(buf, 1, sizeof(buf)-1, fp); buf[n] = '\0'; pclose(fp); }
        for (size_t i = 0; i < n; i++) { if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = '\0'; break; } }
    }
    if (buf[0]) { snprintf(cache, sizeof(cache), "%s", buf); cache_ts = now; snprintf(out, out_sz, "%s", buf); }
    else out[0] = '\0';
}

void format_uptime(long seconds, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    if (seconds < 0) { snprintf(out, out_sz, "N/A"); return; }
    long s = seconds;
    long years = s / (365L*24L*3600L); s %= (365L*24L*3600L);
    long weeks = s / (7L*24L*3600L);   s %= (7L*24L*3600L);
    long days  = s / (24L*3600L);      s %= (24L*3600L);
    long hours = s / 3600L;            s %= 3600L;
    long mins  = s / 60L;

    char buf[64]; buf[0] = '\0';
    int n = 0;
    if (years > 0)  n += snprintf(buf + n, sizeof(buf) - n, "%ldy ", years);
    if (weeks > 0)  n += snprintf(buf + n, sizeof(buf) - n, "%ldw ", weeks);
    if (days > 0)   n += snprintf(buf + n, sizeof(buf) - n, "%ldd ", days);
    if (hours > 0)  n += snprintf(buf + n, sizeof(buf) - n, "%ldh ", hours);
    if (mins > 0)   n += snprintf(buf + n, sizeof(buf) - n, "%ldm ", mins);
    if (n == 0)     n += snprintf(buf + n, sizeof(buf) - n, "0m ");
    // trim trailing space
    if (n > 0 && buf[n-1] == ' ') buf[n-1] = '\0';
    snprintf(out, out_sz, "%s", buf);
}

// simple weather fetcher using wttr.in; cached to avoid frequent network calls
void fetch_city_cached(char *out, size_t out_sz) {
    static char city_cache[64];
    static time_t city_cache_ts = 0;
    time_t now = time(NULL);
    if (city_cache[0] != '\0' && (now - city_cache_ts) < 1800) { // 30 min cache
        snprintf(out, out_sz, "%s", city_cache);
        return;
    }
    // try ipinfo.io first (plain text). 2s timeout
    FILE *fp = popen("curl -m 2 -s https://ipinfo.io/city", "r");
    if (!fp) { out[0] = '\0'; return; }
    char buf[80]; size_t n = fread(buf, 1, sizeof(buf) - 1, fp); buf[n] = '\0'; pclose(fp);
    // trim newline/CR and spaces
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = '\0'; break; }
    }
    // basic sanitize: if empty or too short, give up
    if (buf[0] == '\0' || strlen(buf) < 2) {
        out[0] = '\0';
        return;
    }
    snprintf(city_cache, sizeof(city_cache), "%s", buf);
    city_cache_ts = now;
    snprintf(out, out_sz, "%s", buf);
}

void fetch_weather_cached(char *out, size_t out_sz) {
    static char cache[128];
    static time_t cache_ts = 0;
    time_t now = time(NULL);
    if (cache[0] != '\0' && (now - cache_ts) < 600) { // 10 min cache
        snprintf(out, out_sz, "%s", cache);
        return;
    }
    const char *city = getenv("SYSMON_WEATHER_CITY");
    char cmd[512];
    char autod_city[64]; autod_city[0] = '\0';
    if (!city || !city[0]) {
        fetch_city_cached(autod_city, sizeof(autod_city));
    }
    // Enhanced weather format with more data
    if (city && city[0]) {
        snprintf(cmd, sizeof(cmd), "curl -m 5 -A 'curl/7' -s 'https://wttr.in/%s?format=%%l:+%%t+(%%f)+%%C+%%h+%%w'", city);
    } else if (autod_city[0]) {
        snprintf(cmd, sizeof(cmd), "curl -m 5 -A 'curl/7' -s 'https://wttr.in/%s?format=%%l:+%%t+(%%f)+%%C+%%h+%%w'", autod_city);
    } else {
        snprintf(cmd, sizeof(cmd), "curl -m 5 -A 'curl/7' -s 'https://wttr.in?format=%%l:+%%t+(%%f)+%%C+%%h+%%w'");
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        snprintf(out, out_sz, "🌡️ Нет данных");
        return;
    }
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    buf[n] = '\0';
    pclose(fp);
    // sanitize newline
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = '\0'; break; }
    }
    // Retry without city on empty or error-ish response
    if (n == 0 || strncmp(buf, "Unknown location", 16) == 0 || strncmp(buf, "ERROR", 5) == 0) {
        snprintf(cmd, sizeof(cmd), "curl -m 5 -A 'curl/7' -s 'https://wttr.in?format=3'");
        fp = popen(cmd, "r");
        if (fp) {
            n = fread(buf, 1, sizeof(buf) - 1, fp);
            buf[n] = '\0';
            pclose(fp);
            for (size_t i = 0; i < n; i++) {
                if (buf[i] == '\n' || buf[i] == '\r') { buf[i] = '\0'; break; }
            }
        }
    }
    // Fallback to open-meteo using IP-based coords from ipinfo.io/loc
    if (n == 0) {
        char loc[64]; size_t ln = 0; loc[0] = '\0';
        fp = popen("curl -m 3 -s https://ipinfo.io/loc", "r");
        if (fp) { ln = fread(loc, 1, sizeof(loc)-1, fp); loc[ln] = '\0'; pclose(fp); }
        for (size_t i = 0; i < ln; i++) { if (loc[i] == '\n' || loc[i] == '\r') { loc[i] = '\0'; break; } }
        if (loc[0]) {
            // Parse "lat,lon"
            char lat[32]={0}, lon[32]={0};
            const char *comma = strchr(loc, ',');
            if (comma) {
                size_t la = (size_t)(comma - loc); if (la > sizeof(lat)-1) la = sizeof(lat)-1; memcpy(lat, loc, la); lat[la]='\0';
                snprintf(lon, sizeof(lon), "%s", comma+1);
                snprintf(cmd, sizeof(cmd),
                        "curl -m 5 -A 'curl/7' -s 'https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current_weather=true&timezone=auto'",
                        lat, lon);
                fp = popen(cmd, "r");
                if (fp) {
                    n = fread(buf, 1, sizeof(buf)-1, fp); buf[n]='\0'; pclose(fp);
                    // very light JSON parse: extract temperature and windspeed numbers
                    // temperature":12.3
                    double t = 0.0, w = 0.0; int got = 0;
                    char *p = strstr(buf, "\"temperature\"");
                    if (p) { p = strchr(p, ':'); if (p) { t = atof(p+1); got++; } }
                    p = strstr(buf, "\"windspeed\"");
                    if (p) { p = strchr(p, ':'); if (p) { w = atof(p+1); got++; } }
                    if (got) {
                        snprintf(buf, sizeof(buf), "%.1f°C, wind %.0f m/s", t, w);
                        n = strlen(buf);
                    } else {
                        n = 0;
                    }
                }
            }
        }
    }
    if (n == 0) { snprintf(out, out_sz, "(N/A)"); return; }
    // store to cache
    snprintf(cache, sizeof(cache), "%s", buf);
    cache_ts = now;
    snprintf(out, out_sz, "%s", buf);
}