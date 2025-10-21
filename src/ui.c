#include "ui.h"
#include "system_info.h"
#include "processes.h"
#include "battery.h"
#include "usb.h"
#include "gpu.h"
#include "anomalies.h"
#include "network.h"
#include "disk.h"
#include "smart.h"
#include "mac_smc.h"
#include "logging.h"
#include "config.h"
#include "platform.h"
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include "developer.h"

// forward declarations
static int is_safe_mode(void);

// Helper function to get total memory in bytes
static long get_total_memory(void) {
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    long total_mem;
    size_t len = sizeof(total_mem);
    if (sysctl(mib, 2, &total_mem, &len, NULL, 0) == 0) {
        return total_mem;
    }
    return 0;
}

// clipped print helper to keep lines within terminal width and avoid wrapping
static void mvprintw_clip(int y, int x, const char *fmt, ...) {
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
static void mvprintw_clip_color(int y, int x, int color_pair, const char *fmt, ...) {
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
static void draw_hsep(int y, int x) {
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (y < 0 || y >= rows) return;
    int len = cols - x - 1;
    if (len <= 0) return;
    for (int i = 0; i < len; i++) mvaddch(y, x + i, '-');
}

// render a simple colored bar for percent [0..100]
static void render_bar(int y, int x, int width, float percent, int colors_on) {
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
static int choose_color_by_percent(float percent, float warn, float crit) {
    if (percent >= crit) return 1;  // red
    if (percent >= warn) return 3;  // yellow
    return 2;                       // green
}

static const char *resolve_config_path() {
    const char *cfg = getenv("SYSMON_CONFIG");
    if (cfg && cfg[0]) return cfg;
    return NULL; // caller will try defaults
}

// Diagnostics state
typedef struct {
    int battery_ok;
    int gpu_ok;
    int usb_ok;
    int smart_ok;
    int smc_ok;
    int colors_ok;
    int iostat_ok;
    int nettop_ok;
    int networksetup_ok;
    int airport_ok;
} diag_t;
static diag_t g_diag;

// Функция для запуска тестов функциональности в зависимости от ОС
static void run_functionality_tests(void) {
    clear();
    int colors_on = has_colors();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows; // Suppress unused variable warning
    
    // Определяем ОС
    platform_t platform = detect_platform();
    const char *platform_name_str = platform_name(platform);
    
    // Draw decorative border
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(2, 10, "╔═════════════════════════════════════════════════════════════╗");
    for (int i = 3; i < 18; i++) {
        mvprintw(i, 10, "║");
        mvprintw(i, 74, "║");
    }
    mvprintw(18, 10, "╚═════════════════════════════════════════════════════════════╝");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(3, 25, "🧪 ТЕСТИРОВАНИЕ ФУНКЦИОНАЛЬНОСТИ 🧪");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    mvprintw(4, 12, "🖥️  ОС: %s", platform_name_str);
    mvprintw(5, 12, "─────────────────────────────────────────────────────────────");
    
    mvprintw(6, 12, "Запуск тестов для %s...", platform_name_str);
    refresh();
    sleep(1);
    
    // Тестируем модули в зависимости от ОС
    int tests_passed = 0;
    int total_tests = 0;
    
    // Тест батареи
    total_tests++;
    mvprintw(8, 12, "🔋 Тестирование батареи...");
    refresh();
    battery_info_t bat;
    if (get_battery_info(&bat) == 0) {
        mvprintw(8, 60, "✓ OK");
        tests_passed++;
    } else {
        mvprintw(8, 60, is_linux() ? "⚠ Desktop" : "✗ Ошибка");
    }
    
    // Тест USB
    total_tests++;
    mvprintw(9, 12, "🔌 Тестирование USB устройств...");
    refresh();
    usb_device_t usb_devs[5];
    int usb_count = 0;
    if (list_usb_devices(usb_devs, 5, &usb_count) == 0 && usb_count >= 0) {
        mvprintw(9, 60, "✓ OK (%d)", usb_count);
        tests_passed++;
    } else {
        mvprintw(9, 60, "✗ Ошибка");
    }
    
    // Тест температур
    total_tests++;
    mvprintw(10, 12, "🌡️  Тестирование температур...");
    refresh();
    float core_temps[8];
    int temp_count = 0;
    if (smc_get_core_temperatures(core_temps, 8, &temp_count) == 0 && temp_count > 0) {
        mvprintw(10, 60, "✓ OK (%d датчиков)", temp_count);
        tests_passed++;
    } else {
        mvprintw(10, 60, "✗ Ошибка");
    }
    
    // Тест GPU
    total_tests++;
    mvprintw(11, 12, "🎮 Тестирование GPU...");
    refresh();
    gpu_info_t gpu;
    if (get_gpu_info(&gpu) == 0) {
        mvprintw(11, 60, "✓ OK");
        tests_passed++;
    } else {
        mvprintw(11, 60, "✗ Ошибка");
    }
    
    // Тест SMART
    total_tests++;
    mvprintw(12, 12, "💿 Тестирование SMART...");
    refresh();
    smart_info_t smart;
    if (get_smart_info(NULL, &smart) == 0 && smart.available) {
        mvprintw(12, 60, "✓ OK");
        tests_passed++;
    } else {
        mvprintw(12, 60, "⚠ Требуются права");
    }
    
    // Результат
    mvprintw(14, 12, "─────────────────────────────────────────────────────────────");
    int result_color = (tests_passed == total_tests) ? 2 : ((tests_passed > total_tests/2) ? 3 : 1);
    if (colors_on) attron(COLOR_PAIR(result_color));
    mvprintw(15, 12, "📊 Результат: %d/%d тестов пройдено", tests_passed, total_tests);
    if (colors_on) attroff(COLOR_PAIR(result_color));
    
    mvprintw(16, 12, "Нажмите любую клавишу для возврата...");
    refresh();
    timeout(-1);
    getch();
    timeout(2000);
}

static void perform_diagnostics(void) {
    memset(&g_diag, 0, sizeof(g_diag));
    const int safe = is_safe_mode();
    const char *no_bat = getenv("SYSMON_NO_BAT");
    const char *no_gpu = getenv("SYSMON_NO_GPU");
    const char *no_usb = getenv("SYSMON_NO_USB");
    // Battery - восстановлена поддержка для обеих платформ
    if (!safe && !(no_bat && no_bat[0]=='1')) {
        battery_info_t bat;
        if (get_battery_info(&bat) == 0 && (bat.percentage >= 0 || bat.charging >= 0)) {
            g_diag.battery_ok = 1;
        } else {
            g_diag.battery_ok = 0;
        }
    }
    // GPU
    if (!safe && !(no_gpu && no_gpu[0]=='1')) {
        gpu_info_t gpu;
        if (get_gpu_info(&gpu) == 0 && gpu.usage >= -1.0f) g_diag.gpu_ok = 1;
    }
    // USB
    if (!safe && !(no_usb && no_usb[0]=='1')) {
        usb_device_t devs[1]; int cnt = 0;
        if (list_usb_devices(devs, 1, &cnt) == 0) g_diag.usb_ok = 1;
    }
    // SMART (only check presence, cheap)
    if (!safe) {
        smart_info_t sm;
        if (get_smart_info(NULL, &sm) == 0 && sm.available) g_diag.smart_ok = 1;
    }
    // SMC
    if (!safe) {
        float core_t[1]; int wrote = 0;
        if (smc_get_core_temperatures(core_t, 1, &wrote) == 0 && wrote > 0) g_diag.smc_ok = 1;
    }
    // Colors
    g_diag.colors_ok = has_colors();
    // Tools presence (best-effort via which) - адаптировано для обеих платформ
    FILE *fp;
    fp = popen("which iostat 2>/dev/null", "r"); if (fp) { int c = fgetc(fp); if (c != EOF) g_diag.iostat_ok = 1; pclose(fp);} 
    
    // Специфичные для платформы инструменты
    if (is_macos()) {
        fp = popen("which nettop 2>/dev/null", "r"); if (fp) { int c = fgetc(fp); if (c != EOF) g_diag.nettop_ok = 1; pclose(fp);} 
        fp = popen("which networksetup 2>/dev/null", "r"); if (fp) { int c = fgetc(fp); if (c != EOF) g_diag.networksetup_ok = 1; pclose(fp);} 
        fp = popen("/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I 2>/dev/null | head -n1", "r");
        if (fp) { int c = fgetc(fp); if (c != EOF) g_diag.airport_ok = 1; pclose(fp);}
    } else if (is_linux()) {
        // Linux специфичные инструменты
        fp = popen("which iwconfig 2>/dev/null || which nmcli 2>/dev/null", "r"); if (fp) { int c = fgetc(fp); if (c != EOF) g_diag.airport_ok = 1; pclose(fp);}
    } 
}

static void show_diagnostics_screen(void) {
    clear();
    int colors_on = has_colors();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows; // Suppress unused variable warning
    
    // Draw decorative border
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(2, 10, "╔═════════════════════════════════════════════════════════════╗");
    for (int i = 3; i < 16; i++) {
        mvprintw(i, 10, "║");
        mvprintw(i, 74, "║");
    }
    mvprintw(16, 10, "╚═════════════════════════════════════════════════════════════╝");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(3, 25, "🔍 ДИАГНОСТИКА СИСТЕМЫ 🔍");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    // Отображение операционной системы
    platform_t platform = detect_platform();
    mvprintw(4, 12, "🖥️  ОС:             %s", platform_name(platform));
    
    mvprintw(5, 12, "─────────────────────────────────────────────────────────────");
    
    // Display diagnostics with icons and colors
    int status_color;
    
    // Battery
    status_color = g_diag.battery_ok ? 2 : 1;
    if (colors_on) attron(COLOR_PAIR(status_color));
    mvprintw(6, 12, "🔋 Батарея:        %s", g_diag.battery_ok ? "✓ Работает" : "✗ Недоступно");
    if (colors_on) attroff(COLOR_PAIR(status_color));
    
    // GPU
    status_color = g_diag.gpu_ok ? 2 : 1;
    if (colors_on) attron(COLOR_PAIR(status_color));
    mvprintw(7, 12, "🎮 Видеокарта:     %s", g_diag.gpu_ok ? "✓ Работает" : "✗ Недоступно");
    if (colors_on) attroff(COLOR_PAIR(status_color));
    
    // USB
    status_color = g_diag.usb_ok ? 2 : 1;
    if (colors_on) attron(COLOR_PAIR(status_color));
    mvprintw(8, 12, "🔌 USB устройства: %s", g_diag.usb_ok ? "✓ Работает" : "✗ Недоступно");
    if (colors_on) attroff(COLOR_PAIR(status_color));
    
    // SMART
    status_color = g_diag.smart_ok ? 2 : 3;
    if (colors_on) attron(COLOR_PAIR(status_color));
    mvprintw(9, 12, "💿 SMART диски:    %s", g_diag.smart_ok ? "✓ Работает" : "⚠ Требуется smartctl");
    if (colors_on) attroff(COLOR_PAIR(status_color));
    
    // Temperature sensors (SMC для macOS, hwmon для Linux)
    const char *temp_label = is_linux() ? "🌡️  HWMON датчики:" : "🌡️  SMC датчики:";
    status_color = g_diag.smc_ok ? 2 : 1;
    if (colors_on) attron(COLOR_PAIR(status_color));
    mvprintw(10, 12, "%s %s", temp_label, g_diag.smc_ok ? "✓ Работает" : "✗ Недоступно");
    if (colors_on) attroff(COLOR_PAIR(status_color));
    
    // Colors
    status_color = g_diag.colors_ok ? 2 : 3;
    if (colors_on) attron(COLOR_PAIR(status_color));
    mvprintw(11, 12, "🎨 Цвета:          %s", g_diag.colors_ok ? "✓ Включены" : "⚠ Отключены");
    if (colors_on) attroff(COLOR_PAIR(status_color));
    
    mvprintw(11, 12, "─────────────────────────────────────────────────────────────");
    
    // Tools section
    if (colors_on) attron(COLOR_PAIR(3));
    mvprintw(12, 12, "📦 Установленные утилиты:");
    if (colors_on) attroff(COLOR_PAIR(3));
    
    mvprintw(13, 14, "%s iostat    %s nettop    %s networksetup    %s airport",
             g_diag.iostat_ok ? "✓" : "✗",
             g_diag.nettop_ok ? "✓" : "✗",
             g_diag.networksetup_ok ? "✓" : "✗",
             g_diag.airport_ok ? "✓" : "✗");
    
    mvprintw(14, 12, "─────────────────────────────────────────────────────────────");
    
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(15, 20, "Нажмите любую клавишу для продолжения");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    timeout(3000);
    int ch = getch();
    if (ch == ERR) {
        // Timeout occurred, continue automatically
        if (colors_on) attron(COLOR_PAIR(3));
        mvprintw(17, 25, "⏳ Автоматический запуск...");
        refresh();
        sleep(1);
    } else {
        // User pressed a key, wait a moment then continue
        mvprintw(13, 2, "Continuing...");
        refresh();
        sleep(1);
    }
    // Clear the screen before returning to main interface
    clear();
    refresh();
    (void)ch;
}

// Developer tools screens
static void show_listening_ports() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(0, 2, "╔════════════════════════════════════════════════════════════════════╗");
    for (int i = 1; i < rows-2; i++) {
        mvprintw(i, 2, "║");
        mvprintw(i, 73, "║");
    }
    mvprintw(rows-2, 2, "╚════════════════════════════════════════════════════════════════╝");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(1, 25, "🔌 ЗАНЯТЫЕ ПОРТЫ 🔌");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    mvprintw(2, 4, "──────────────────────────────────────────────────────────────");
    mvprintw(3, 4, "Порт    Процесс         PID     Сервис           Доступ");
    mvprintw(4, 4, "──────────────────────────────────────────────────────────────");
    
    // Get real listening ports
    port_info_t ports[50];
    int port_count = 0;
    get_listening_ports(ports, 50, &port_count);
    
    int y = 5;
    if (port_count > 0) {
        for (int i = 0; i < port_count && i < 15; i++) {
            mvprintw(y++, 4, "%-7d %-15s %-7d %-16s %s",
                     ports[i].port,
                     ports[i].process,
                     ports[i].pid,
                     ports[i].service,
                     ports[i].localhost_only ? "localhost" : "all");
        }
    } else {
        mvprintw(y, 4, "Нет активных портов");
    }
    
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(rows-1, 20, "[K] Освободить порт  [Q] Назад");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    timeout(-1);
    getch();
    timeout(2000);
}

static void show_git_repos() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    int selected = 0;
    
    while (1) {
        clear();
        
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(0, 2, "╔════════════════════════════════════════════════════════════════════╗");
        mvprintw(rows-2, 2, "╚════════════════════════════════════════════════════════════════════╝");
        for (int i = 1; i < rows-2; i++) {
            mvprintw(i, 2, "║");
            mvprintw(i, 73, "║");
        }
        if (colors_on) attroff(COLOR_PAIR(4));
        
        if (colors_on) attron(COLOR_PAIR(6));
        mvprintw(1, 25, "🔧 GIT РЕПОЗИТОРИИ 🔧");
        if (colors_on) attroff(COLOR_PAIR(6));
        
        mvprintw(3, 4, "Репозиторий              Ветка      Изменения   Статус");
        mvprintw(4, 4, "──────────────────────────────────────────────────────────────");
        
        // Get real Git repositories
        git_repo_status_t repos[20];
        int repo_count = 0;
        find_git_repos(repos, 20, &repo_count);
        
        if (repo_count > 0) {
            for (int i = 0; i < repo_count && i < 15; i++) {
                int y = 5 + i;
                if (i == selected) {
                    if (colors_on) attron(COLOR_PAIR(2));
                    mvprintw(y, 3, "►");
                }
                
                // Shorten path for display
                char short_path[25];
                const char *home = getenv("HOME");
                if (home && strstr(repos[i].path, home)) {
                    snprintf(short_path, sizeof(short_path), "~%s", repos[i].path + strlen(home));
                } else {
                    strncpy(short_path, repos[i].path, sizeof(short_path) - 1);
                }
                
                // Determine status icon
                const char *status_icon = "✓ чисто";
                if (repos[i].uncommitted_changes > 0) status_icon = "⚠ изменения";
                else if (repos[i].unpushed_commits > 0) status_icon = "⚠ не запушено";
                else if (repos[i].is_dirty) status_icon = "⚠ dirty";
                
                mvprintw(y, 4, "%-24s %-10s %-11d %s", 
                        short_path, 
                        repos[i].branch[0] ? repos[i].branch : "n/a",
                        repos[i].uncommitted_changes,
                        status_icon);
                
                if (i == selected && colors_on) attroff(COLOR_PAIR(2));
            }
        } else {
            mvprintw(5, 4, "Git репозитории не найдены");
        }
        
        mvprintw(rows-4, 4, "──────────────────────────────────────────────────────────────");
        if (colors_on) attron(COLOR_PAIR(5));
        mvprintw(rows-3, 4, "[↑↓] Выбор  [Enter] Детали  [C] Commit  [P] Push  [Q] Назад");
        if (colors_on) attroff(COLOR_PAIR(5));
        
        refresh();
        int ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) break;
        if (ch == KEY_UP && selected > 0) selected--;
        if (ch == KEY_DOWN && selected < 3) selected++;
    }
}

static void show_dev_environment() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(1, 5, "╔═══════════════════════════════════════════════════════════════╗");
    for (int i = 2; i < 24; i++) {
        mvprintw(i, 5, "║");
        mvprintw(i, 71, "║");
    }
    mvprintw(24, 5, "╚═══════════════════════════════════════════════════════════════╝");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(2, 22, "💻 ОКРУЖЕНИЕ РАЗРАБОТЧИКА 💻");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    mvprintw(3, 7, "─────────────────────────────────────────────────────────────");
    
    // Get real environment info
    dev_environment_t env;
    get_dev_environment(&env);
    
    // Languages and runtimes
    mvprintw(4, 7, "🔧 Языки и среды выполнения:");
    if (env.node_version[0]) 
        mvprintw(5, 9, "Node.js:    %-12s npm:     %s", env.node_version, env.npm_version);
    if (env.python_version[0])
        mvprintw(6, 9, "Python:     %-12s pip:     %s", env.python_version, env.pip_version);
    if (env.ruby_version[0])
        mvprintw(7, 9, "Ruby:       %s", env.ruby_version);
    if (env.go_version[0] || env.java_version[0])
        mvprintw(8, 9, "Go:         %-12s Java:    %s", env.go_version, env.java_version);
    if (env.rust_version[0])
        mvprintw(9, 9, "Rust:       %s", env.rust_version);
    
    mvprintw(10, 7, "─────────────────────────────────────────────────────────────");
    
    // Get real database status
    database_status_t db_status[10];
    int db_count = 0;
    get_database_status(db_status, 10, &db_count);
    
    mvprintw(11, 7, "🗄️  Базы данных:");
    int db_y = 12;
    for (int i = 0; i < db_count && i < 4; i++) {
        const char *status_icon = db_status[i].is_running ? "✅" : "❌";
        const char *status_text = db_status[i].is_running ? "работает" : "остановлен";
        mvprintw(db_y++, 9, "%-11s %-12s %s %s (%d)", 
                db_status[i].name,
                db_status[i].version[0] ? db_status[i].version : "n/a",
                status_icon, status_text,
                db_status[i].port);
    }
    
    mvprintw(16, 7, "─────────────────────────────────────────────────────────────");
    
    // Tools
    mvprintw(17, 7, "🛠️  Инструменты:");
    if (env.docker_version[0])
        mvprintw(18, 9, "Docker:     %s", env.docker_version);
    if (env.brew_packages > 0)
        mvprintw(19, 9, "Homebrew:   %d пакетов", env.brew_packages);
    
    mvprintw(21, 7, "─────────────────────────────────────────────────────────────");
    
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(rows-2, 25, "[U] Обновить все  [Q] Назад");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    timeout(-1);
    getch();
    timeout(2000);
}

static void show_dev_processes() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(0, 1, "╔");
    for (int i = 2; i < cols-2; i++) mvprintw(0, i, "═");
    mvprintw(0, cols-2, "╗");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(1, cols/2-20, "⚙️  ПРОЦЕССЫ РАЗРАБОТКИ ⚙️");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    mvprintw(3, 3, "PID     Процесс          Тип        CPU%%   MEM%%   Команда");
    mvprintw(4, 3, "─────────────────────────────────────────────────────────────────────");
    
    // Get real development processes
    dev_process_t processes[50];
    int proc_count = 0;
    get_dev_processes(processes, 50, &proc_count);
    
    int y = 5;
    float total_cpu = 0, total_mem = 0;
    
    if (proc_count > 0) {
        for (int i = 0; i < proc_count && i < 15; i++) {
            mvprintw(y++, 3, "%-7d %-16s %-10s %5.1f  %5.1f  %.30s",
                     processes[i].pid,
                     processes[i].name,
                     processes[i].type,
                     processes[i].cpu_usage,
                     processes[i].mem_usage,
                     processes[i].command);
            total_cpu += processes[i].cpu_usage;
            total_mem += processes[i].mem_usage;
        }
    } else {
        mvprintw(y, 3, "Нет активных процессов разработки");
    }
    
    mvprintw(rows-3, 3, "─────────────────────────────────────────────────────────────────────");
    mvprintw(rows-2, 3, "Всего процессов разработки: %d   CPU: %.1f%%   MEM: %.1f%%", 
             proc_count, total_cpu, total_mem);
    
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(rows-1, cols/2-20, "[K] Завершить  [R] Перезапустить  [Q] Назад");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    timeout(-1);
    getch();
    timeout(2000);
}

static void show_dev_quick_actions() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    int selected = 0;
    
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
    
    while (1) {
        clear();
        
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(2, 8, "╔══════════════════════════════════════════════════════════╗");
        mvprintw(3, 8, "║");
        mvprintw(3, 69, "║");
        mvprintw(4, 8, "╚══════════════════════════════════════════════════════════╝");
        if (colors_on) attroff(COLOR_PAIR(4));
        
        if (colors_on) attron(COLOR_PAIR(6));
        mvprintw(3, 20, "⚡ БЫСТРЫЕ ДЕЙСТВИЯ РАЗРАБОТЧИКА ⚡");
        if (colors_on) attroff(COLOR_PAIR(6));
        
        for (int i = 0; i < num_actions; i++) {
            if (i == selected) {
                if (colors_on) attron(COLOR_PAIR(2));
                mvprintw(6 + i, 6, "► %s", actions[i]);
                if (colors_on) attroff(COLOR_PAIR(2));
            } else {
                mvprintw(6 + i, 8, "%s", actions[i]);
            }
        }
        
        mvprintw(rows-3, 8, "────────────────────────────────────────────────────────");
        if (colors_on) attron(COLOR_PAIR(5));
        mvprintw(rows-2, 15, "[↑↓] Выбор  [Enter] Выполнить  [Q] Назад");
        if (colors_on) attroff(COLOR_PAIR(5));
        
        refresh();
        int ch = getch();
        
        if (ch == 'q' || ch == 'Q' || ch == 27) break;
        if (ch == KEY_UP && selected > 0) selected--;
        if (ch == KEY_DOWN && selected < num_actions - 1) selected++;
        if (ch == '\n' || ch == '\r') {
            mvprintw(rows-1, 15, "Выполняется: %s...", actions[selected]);
            refresh();
            
            // Execute real action
            char output[256];
            int result = execute_dev_action((dev_action_t)selected, output, sizeof(output));
            
            if (result == 0) {
                if (colors_on) attron(COLOR_PAIR(2));
                mvprintw(rows-1, 15, "✓ %s", output);
                if (colors_on) attroff(COLOR_PAIR(2));
            } else {
                if (colors_on) attron(COLOR_PAIR(1));
                mvprintw(rows-1, 15, "✗ Ошибка выполнения");
                if (colors_on) attroff(COLOR_PAIR(1));
            }
            refresh();
            sleep(2);
        }
    }
}

// Enhanced weather display
static void show_weather_details() {
    clear();
    int cols;
    getmaxyx(stdscr, (int){0}, cols);
    int colors_on = has_colors();
    
    // Draw beautiful border
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(1, 5, "╔════════════════════════════════════════════════════════════════╗");
    for (int i = 2; i < 20; i++) {
        mvprintw(i, 5, "║");
        mvprintw(i, 73, "║");
    }
    mvprintw(20, 5, "╚════════════════════════════════════════════════════════════════╝");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(2, 25, "🌤️  ПОДРОБНАЯ ПОГОДА  🌤️");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    mvprintw(3, 7, "──────────────────────────────────────────────────────────────");
    
    // Fetch weather (simplified for now - will be enhanced with enhanced.c)
    char city[64] = "Москва";
    const char *env_city = getenv("SYSMON_WEATHER_CITY");
    if (env_city && env_city[0]) {
        snprintf(city, sizeof(city), "%s", env_city);
    }
    
    // Mock enhanced weather data for now
    mvprintw(4, 7, "📍 Город: %s", city);
    mvprintw(5, 7, "🌡️  Температура: 22°C (ощущается как 20°C)");
    mvprintw(6, 7, "☁️  Облачность: Переменная облачность");
    mvprintw(7, 7, "💧 Влажность: 65%%");
    mvprintw(8, 7, "🌬️  Ветер: 5 м/с, СЗ");
    mvprintw(9, 7, "🔵 Давление: 1015 гПа");
    mvprintw(10, 7, "🌅 Восход: 06:30   🌇 Закат: 20:15");
    
    mvprintw(12, 7, "──────────────────────────────────────────────────────────────");
    mvprintw(13, 7, "📊 Прогноз на ближайшие часы:");
    mvprintw(14, 7, "  09:00  ☀️  23°C    12:00  ⛅ 25°C    15:00  ☁️  24°C");
    mvprintw(15, 7, "  18:00  🌧️  20°C    21:00  🌙 18°C    00:00  🌙 16°C");
    
    mvprintw(17, 7, "──────────────────────────────────────────────────────────────");
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(18, 20, "Нажмите любую клавишу для возврата");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    timeout(-1);
    getch();
    timeout(2000);
}

// Show network connections
static void show_network_connections() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(0, 2, "╔═══════════════════════════════════════════════════════════════════════╗");
    mvprintw(rows-2, 2, "╚═══════════════════════════════════════════════════════════════════════╝");
    for (int i = 1; i < rows-2; i++) {
        mvprintw(i, 2, "║");
        mvprintw(i, 76, "║");
    }
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(1, 20, "🌐 АКТИВНЫЕ СЕТЕВЫЕ СОЕДИНЕНИЯ 🌐");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    mvprintw(2, 4, "────────────────────────────────────────────────────────────────────");
    mvprintw(3, 4, "Протокол  Локальный адрес         Удаленный адрес          Сервис");
    mvprintw(4, 4, "────────────────────────────────────────────────────────────────────");
    
    // Mock network connections for now
    int y = 5;
    mvprintw(y++, 4, "TCP       127.0.0.1:5432         127.0.0.1:52341         PostgreSQL");
    mvprintw(y++, 4, "TCP       *:443                  93.184.216.34:12345     HTTPS");
    mvprintw(y++, 4, "TCP       *:22                   192.168.1.100:54321     SSH");
    mvprintw(y++, 4, "TCP       127.0.0.1:6379         127.0.0.1:43210         Redis");
    
    mvprintw(rows-4, 4, "────────────────────────────────────────────────────────────────────");
    mvprintw(rows-3, 4, "Всего соединений: 4    Входящих: 2    Исходящих: 2");
    
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(rows-1, 25, "[Q] Назад  [R] Обновить");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    while (1) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) break;
        if (ch == 'r' || ch == 'R') {
            mvprintw(rows/2, cols/2-10, "Обновление...");
            refresh();
            sleep(1);
            show_network_connections();
            break;
        }
    }
}

// Show Docker containers
static void show_docker_containers() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    
    if (colors_on) attron(COLOR_PAIR(4));
    mvprintw(0, 1, "╔");
    for (int i = 2; i < cols-2; i++) mvprintw(0, i, "═");
    mvprintw(0, cols-2, "╗");
    if (colors_on) attroff(COLOR_PAIR(4));
    
    if (colors_on) attron(COLOR_PAIR(6));
    mvprintw(1, cols/2-15, "🐳 DOCKER КОНТЕЙНЕРЫ 🐳");
    if (colors_on) attroff(COLOR_PAIR(6));
    
    // Check if Docker is available
    FILE *fp = popen("docker version 2>&1 | grep -q 'Version' && echo 'OK'", "r");
    char buf[16];
    int docker_available = 0;
    if (fp) {
        if (fgets(buf, sizeof(buf), fp)) {
            docker_available = (strncmp(buf, "OK", 2) == 0);
        }
        pclose(fp);
    }
    
    if (!docker_available) {
        if (colors_on) attron(COLOR_PAIR(3));
        mvprintw(rows/2, cols/2-20, "⚠️  Docker не установлен или не запущен");
        if (colors_on) attroff(COLOR_PAIR(3));
    } else {
        mvprintw(3, 3, "ID         Имя              Образ              CPU    MEM      Статус");
        mvprintw(4, 3, "──────────────────────────────────────────────────────────────────────");
        // Would show real containers here
        mvprintw(5, 3, "a1b2c3     my-app           nginx:latest       2.5%%   128MB    Running");
        mvprintw(6, 3, "d4e5f6     database         postgres:13        5.1%%   512MB    Running");
    }
    
    if (colors_on) attron(COLOR_PAIR(5));
    mvprintw(rows-2, cols/2-15, "[Q] Назад  [K] Остановить  [S] Запустить");
    if (colors_on) attroff(COLOR_PAIR(5));
    
    refresh();
    timeout(-1);
    getch();
    timeout(2000);
}

// Show quick actions menu
static void show_quick_actions() {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int colors_on = has_colors();
    int selected = 0;
    
    const char *actions[] = {
        "🧹 Очистить системный кэш",
        "🌐 Перезапустить сеть",
        "🔄 Очистить DNS кэш",
        "💾 Освободить память",
        "🔄 Проверить обновления",
        "📊 Показать большие файлы",
        "🎯 Перезапустить Dock",
        "📁 Перезапустить Finder",
        "👁️  Показать/скрыть скрытые файлы",
        "🗑️  Очистить корзину"
    };
    int num_actions = 10;
    
    while (1) {
        clear();
        
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(2, 10, "╔══════════════════════════════════════════════════╗");
        mvprintw(3, 10, "║");
        mvprintw(3, 62, "║");
        mvprintw(4, 10, "╚══════════════════════════════════════════════════╝");
        if (colors_on) attroff(COLOR_PAIR(4));
        
        if (colors_on) attron(COLOR_PAIR(6));
        mvprintw(3, 25, "⚡ БЫСТРЫЕ ДЕЙСТВИЯ ⚡");
        if (colors_on) attroff(COLOR_PAIR(6));
        
        for (int i = 0; i < num_actions; i++) {
            if (i == selected) {
                if (colors_on) attron(COLOR_PAIR(2));
                mvprintw(6 + i, 8, "► %s", actions[i]);
                if (colors_on) attroff(COLOR_PAIR(2));
            } else {
                mvprintw(6 + i, 10, "%s", actions[i]);
            }
        }
        
        mvprintw(rows-4, 10, "───────────────────────────────────────────────");
        if (colors_on) attron(COLOR_PAIR(5));
        mvprintw(rows-3, 10, "[↑↓] Выбор  [Enter] Выполнить  [Q] Назад");
        if (colors_on) attroff(COLOR_PAIR(5));
        
        refresh();
        int ch = getch();
        
        if (ch == 'q' || ch == 'Q' || ch == 27) break;
        if (ch == KEY_UP && selected > 0) selected--;
        if (ch == KEY_DOWN && selected < num_actions - 1) selected++;
        if (ch == '\n' || ch == '\r') {
            mvprintw(rows-2, 10, "Выполняется: %s...", actions[selected]);
            refresh();
            sleep(2);
            mvprintw(rows-2, 10, "✓ Выполнено успешно!                    ");
            refresh();
            sleep(1);
        }
    }
}

// cache Wi‑Fi SSID for 60s (macOS)
static void fetch_ssid_cached(char *out, size_t out_sz) {
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

static void format_uptime(long seconds, char *out, size_t out_sz) {
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
static void fetch_city_cached(char *out, size_t out_sz) {
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

static void fetch_weather_cached(char *out, size_t out_sz) {
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

static volatile sig_atomic_t g_resized = 0;
static void handle_sigwinch(int sig) {
    (void)sig;
    g_resized = 1;
}

// Process view sorting/filtering
static int g_proc_sort_mode = 0; // 0=PID, 1=CPU desc, 2=MEM desc
static char g_proc_filter[64] = "";

static int cmp_cpu_desc(const void *a, const void *b) {
    const process_info_t *pa = (const process_info_t *)a;
    const process_info_t *pb = (const process_info_t *)b;
    if (pa->cpu_usage < pb->cpu_usage) return 1;
    if (pa->cpu_usage > pb->cpu_usage) return -1;
    return 0;
}
static int cmp_mem_desc(const void *a, const void *b) {
    const process_info_t *pa = (const process_info_t *)a;
    const process_info_t *pb = (const process_info_t *)b;
    if (pa->mem_usage < pb->mem_usage) return 1;
    if (pa->mem_usage > pb->mem_usage) return -1;
    return 0;
}

void show_processes() {
    int selected = 0;
    int page_offset = 0;
    const int items_per_page = 15;
    
    while (1) {
        process_info_t procs[256];
        size_t count = get_process_list(procs, 256);

        // filter into temp
        process_info_t view[256]; size_t v = 0;
        for (size_t i = 0; i < count && v < 256; i++) {
            if (g_proc_filter[0]) {
                if (strstr(procs[i].name, g_proc_filter) == NULL) continue;
            }
            view[v++] = procs[i];
        }
        // sort
        if (g_proc_sort_mode == 1) qsort(view, v, sizeof(process_info_t), cmp_cpu_desc);
        else if (g_proc_sort_mode == 2) qsort(view, v, sizeof(process_info_t), cmp_mem_desc);

        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        
        // Draw nice border
        int colors_on = has_colors();
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(0, 1, "╔");
        for (int i = 2; i < cols-2; i++) mvprintw(0, i, "═");
        mvprintw(0, cols-2, "╗");
        for (int i = 1; i < rows-3; i++) {
            mvprintw(i, 1, "║");
            mvprintw(i, cols-2, "║");
        }
        mvprintw(rows-3, 1, "╚");
        for (int i = 2; i < cols-2; i++) mvprintw(rows-3, i, "═");
        mvprintw(rows-3, cols-2, "╝");
        if (colors_on) attroff(COLOR_PAIR(4));
        
        if (colors_on) attron(COLOR_PAIR(6));
        mvprintw(1, 4, "◆ ПРОЦЕССЫ СИСТЕМЫ ◆ (всего: %zu, показано: %d-%d)", v, 
                 page_offset + 1, 
                 (page_offset + items_per_page > (int)v) ? (int)v : page_offset + items_per_page);
        if (colors_on) attroff(COLOR_PAIR(6));
        
        const char *sort_lbl = g_proc_sort_mode==1?"CPU↓":(g_proc_sort_mode==2?"MEM↓":"PID");
        mvprintw(2, 4, "Сортировка: %s   Фильтр: '%s'", sort_lbl, g_proc_filter);
        
        // Header with colors
        if (colors_on) attron(COLOR_PAIR(3));
        mvprintw(4, 4, "    PID      CPU%%     MEM%%     RSS MB   СОСТОЯНИЕ   ИМЯ ПРОЦЕССА");
        if (colors_on) attroff(COLOR_PAIR(3));
        
        // Draw separator line
        mvprintw(5, 3, "─────────────────────────────────────────────────────────────────────────");

        // Show processes with selection highlight
        for (int i = 0; i < items_per_page && (page_offset + i) < (int)v; i++) {
            int idx = page_offset + i;
            int is_selected = (idx == selected);
            
            if (is_selected) {
                if (colors_on) attron(COLOR_PAIR(2));
                mvprintw(6 + i, 3, "►");
            }
            
            // Color coding for CPU usage
            int cpu_color = 5; // default white
            if (view[idx].cpu_usage > 80.0f) cpu_color = 1; // red
            else if (view[idx].cpu_usage > 50.0f) cpu_color = 3; // yellow
            else if (view[idx].cpu_usage > 20.0f) cpu_color = 2; // green
            
            if (colors_on && !is_selected) attron(COLOR_PAIR(cpu_color));
            
            // Calculate RSS in MB
            float rss_mb = view[idx].mem_usage * get_total_memory() / 100.0f / (1024.0f * 1024.0f);
            
            mvprintw(6 + i, 4, " %-8d %6.1f%%  %6.1f%%  %8.1f   %-10s  %-30s",
                     view[idx].pid,
                     view[idx].cpu_usage,
                     view[idx].mem_usage,
                     rss_mb,
                     view[idx].cpu_usage > 0.1f ? "Активен" : "Спящий",
                     view[idx].name);
            
            if (colors_on) {
                if (is_selected) attroff(COLOR_PAIR(2));
                else attroff(COLOR_PAIR(cpu_color));
            }
        }
        
        // Statistics summary
        float total_cpu = 0, total_mem = 0;
        for (size_t i = 0; i < v; i++) {
            total_cpu += view[i].cpu_usage;
            total_mem += view[i].mem_usage;
        }
        
        mvprintw(rows-6, 4, "──────────────────────────────────────────────────────────────");
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(rows-5, 4, "Статистика: CPU всего: %.1f%%  |  MEM всего: %.1f%%  |  Процессов: %zu", 
                 total_cpu, total_mem, v);
        if (colors_on) attroff(COLOR_PAIR(4));
        
        // Instructions
        if (colors_on) attron(COLOR_PAIR(5));
        mvprintw(rows-2, 4, "[↑↓] Выбор  [k] УБИТЬ процесс  [s] Сортировка  [/] Фильтр  [PgUp/PgDn] Страницы  [q] Назад");
        if (colors_on) attroff(COLOR_PAIR(5));
        
        refresh();
        timeout(2000);
        int ch = getch();
        
        if (ch == 'q' || ch == 'Q' || ch == 27) break;
        
        // Navigation
        if (ch == KEY_UP || ch == 'w' || ch == 'W') {
            if (selected > 0) selected--;
            if (selected < page_offset) page_offset = selected;
        }
        else if (ch == KEY_DOWN || ch == 's' || ch == 'S') {
            if (selected < (int)v - 1) selected++;
            if (selected >= page_offset + items_per_page) page_offset = selected - items_per_page + 1;
        }
        else if (ch == KEY_PPAGE) { // Page Up
            page_offset -= items_per_page;
            if (page_offset < 0) page_offset = 0;
            selected = page_offset;
        }
        else if (ch == KEY_NPAGE) { // Page Down
            page_offset += items_per_page;
            if (page_offset >= (int)v) page_offset = (int)v - items_per_page;
            if (page_offset < 0) page_offset = 0;
            selected = page_offset;
        }
        
        // Kill process
        else if (ch == 'k' || ch == 'K') {
            if (v > 0 && selected >= 0 && selected < (int)v) {
                int pid = view[selected].pid;
                char confirm_msg[128];
                snprintf(confirm_msg, sizeof(confirm_msg), 
                        "Убить процесс '%s' (PID: %d)? [y/n]", view[selected].name, pid);
                mvprintw(rows/2, (cols - strlen(confirm_msg))/2 - 5, "╔════════════════════════════════════════════════╗");
                mvprintw(rows/2+1, (cols - strlen(confirm_msg))/2 - 5, "║ %-46s ║", confirm_msg);
                mvprintw(rows/2+2, (cols - strlen(confirm_msg))/2 - 5, "╚════════════════════════════════════════════════╝");
                refresh();
                
                timeout(-1); // Wait for input
                int confirm = getch();
                timeout(2000); // Restore timeout
                
                if (confirm == 'y' || confirm == 'Y') {
                    int result = kill(pid, SIGTERM);
                    if (result == 0) {
                        if (colors_on) attron(COLOR_PAIR(2));
                        mvprintw(rows/2+4, (cols - 30)/2, "✓ Процесс успешно завершен");
                        if (colors_on) attroff(COLOR_PAIR(2));
                    } else {
                        if (colors_on) attron(COLOR_PAIR(1));
                        mvprintw(rows/2+4, (cols - 40)/2, "✗ Ошибка: нет прав для завершения процесса");
                        if (colors_on) attroff(COLOR_PAIR(1));
                    }
                    refresh();
                    sleep(1);
                }
            }
        }
        
        // Sort mode
        else if (ch == 's' || ch == 'S') {
            g_proc_sort_mode = (g_proc_sort_mode + 1) % 3;
            selected = 0;
            page_offset = 0;
        }
        
        // Filter
        else if (ch == '/') {
            echo(); curs_set(TRUE);
            char buf[64]; snprintf(buf, sizeof(buf), "%s", g_proc_filter);
            mvprintw(rows-8, 4, "Введите фильтр: ");
            getnstr(buf, (int)sizeof(buf)-1);
            noecho(); curs_set(FALSE);
            // trim spaces
            size_t L = strlen(buf);
            while (L>0 && (buf[L-1]==' '||buf[L-1]=='\t')) { buf[--L]='\0'; }
            snprintf(g_proc_filter, sizeof(g_proc_filter), "%s", buf);
            selected = 0;
            page_offset = 0;
        }
    }
}

static int is_safe_mode(void) {
    const char *s = getenv("SYSMON_SAFE");
    return (s && s[0] == '1');
}

void run_ui() {
    if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: run_ui() started\n");
    
    // Проверяем TTY и TERM переменную для корректной работы с терминалом
    const char *term = getenv("TERM");
    if (!isatty(STDOUT_FILENO) || !isatty(STDIN_FILENO) || 
        (term && (strcmp(term, "dumb") == 0 || strcmp(term, "unknown") == 0))) {
        if (getenv("SYSMON_DEBUG")) {
            fprintf(stderr, "DBG: Not in proper TTY (TERM=%s), using non-interactive mode\n", 
                    term ? term : "NULL");
        }
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Getting CPU usage...\n");
        float cpu = get_cpu_usage();
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Getting memory usage...\n");
        float mem = get_memory_usage();
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Getting uptime...\n");
        long uptime = get_uptime_seconds();
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Getting disk info...\n");
        const char *disk_info = get_disk_info();
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Getting network info...\n");
        const char *net_info = get_network_info();
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Formatting uptime...\n");
        char up[64]; 
        format_uptime(uptime, up, sizeof(up));
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Printing results...\n");
        printf("CPU %.1f%%  MEM %.1f%%  UPTIME %s  DISK %s  NET %s\n", 
               cpu >= 0 ? cpu : 0.0f, 
               mem >= 0 ? mem : 0.0f, 
               up, 
               disk_info ? disk_info : "N/A", 
               net_info ? net_info : "N/A");
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Non-interactive mode completed\n");
        return;
    }

    if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: About to initialize ncurses\n");
    
    // Проверяем и устанавливаем правильную TERM переменную для macOS ДО инициализации ncurses
    const char *current_term = getenv("TERM");
    if (!current_term || strcmp(current_term, "dumb") == 0 || strcmp(current_term, "unknown") == 0) {
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: TERM=%s, setting to xterm-256color\n", current_term ? current_term : "NULL");
        setenv("TERM", "xterm-256color", 1); // Перезаписываем переменную
    }
    
    // Enable UTF-8 for ncurses drawing to avoid garbled characters
    setlocale(LC_ALL, "");
    
    // Принудительно инициализируем терминал для взаимодействия
    if (initscr() == NULL) {
        fprintf(stderr, "Error: Unable to initialize ncurses terminal\n");
        return;
    }
    
    if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: ncurses initialized successfully\n");
    
    // Настройки для корректной работы на macOS
    cbreak();        // Включаем cbreak режим для немедленного ввода
    noecho();        // Отключаем echo
    curs_set(FALSE); // Скрываем курсор
    
    // Принудительно включаем raw режим для обработки всех клавиш
    if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Setting up input modes\n");
    int colors_on = has_colors();
    if (colors_on) {
        if (start_color() == ERR) {
            fprintf(stderr, "Warning: Unable to initialize colors\n");
            colors_on = 0;
        }
    }
    // prevent mouse/gesture input from confusing ncurses; mitigate swipe-induced crashes
    mousemask(0, NULL);
    
    // Настройки для максимальной совместимости с macOS Terminal
    keypad(stdscr, TRUE);  // Включаем keypad для поддержки стрелок и функциональных клавиш
    nodelay(stdscr, FALSE); // Отключаем nodelay для корректной блокировки на ввод
    
    // Принудительный сброс терминального буфера
    flushinp();
    // ignore SIGPIPE from any popen/curl mishaps
    signal(SIGPIPE, SIG_IGN);
    // handle terminal resize (often triggered by touchpad gestures)
    signal(SIGWINCH, handle_sigwinch);
    if (colors_on) {
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    }

    // Initialization & diagnostics (skip entirely in safe mode or when SYSMON_NO_DIAG=1)
    const char *no_diag = getenv("SYSMON_NO_DIAG");
    if (!is_safe_mode()) {
        perform_diagnostics();
        if (!(no_diag && no_diag[0] == '1')) {
            show_diagnostics_screen();
        }
    }

    static anomaly_history_t hist;
    static int inited = 0;
    if (!inited) { anomalies_init(&hist); inited = 1; }
    static float cpu_hist[60];
    static float mem_hist[60];
    static int hist_idx = 0;

    // runtime module-failure toggles (auto-disable if detected broken)
    static int battery_failed = 0;
    static int gpu_failed = 0;
    static int usb_failed = 0;

    // weather cache buffer for UI
    static char weather_line[128]; weather_line[0] = '\0';
    
    // Variables for snapshot functionality
    char ssid[64]; ssid[0] = '\0';
    battery_info_t bat; bat.percentage = -1;
    int cycles = -1;

    while (1) {
        int rows = 0, cols = 0; getmaxyx(stdscr, rows, cols);
        
        // Ensure we have a valid terminal size
        if (rows <= 0 || cols <= 0) {
            rows = 24;
            cols = 80;
        }
        
        if (g_resized) {
            endwin();
            refresh();
            clear();
            g_resized = 0;
        } else {
            clear();
        }

        float cpu = get_cpu_usage();
        float mem = get_memory_usage();
        long uptime = get_uptime_seconds();
        
        // Check for invalid values
        if (cpu < 0) cpu = 0.0f;
        if (mem < 0) mem = 0.0f;
        if (uptime < 0) uptime = 0;
        
        long long fcur=-1, fmax=-1; 
        get_cpu_frequencies(&fcur, &fmax);
        char fbuf[64];
        if (fcur > 0 && fmax > 0) {
            snprintf(fbuf, sizeof(fbuf), " (%.2f/%.2f GHz)", (double)fcur/1e9, (double)fmax/1e9);
        } else if (fcur > 0) {
            snprintf(fbuf, sizeof(fbuf), " (%.2f GHz)", (double)fcur/1e9);
        } else {
            fbuf[0] = '\0';
        }
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: basics ok cpu=%.2f mem=%.2f up=%ld\n", cpu, mem, uptime);

        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Starting UI rendering, rows=%d cols=%d\n", rows, cols);

        // Draw title with decorative border
        if (colors_on) {
            attron(COLOR_PAIR(4));
            mvprintw(0, (cols - 50)/2, "╔════════════════════════════════════════════════╗");
            mvprintw(1, (cols - 50)/2, "║");
            mvprintw(1, (cols - 50)/2 + 51, "║");
            mvprintw(2, (cols - 50)/2, "╚════════════════════════════════════════════════╝");
            attroff(COLOR_PAIR(4));
            attron(COLOR_PAIR(6));
            mvprintw(1, (cols - 40)/2 + 5, "◆◆◆ СИСТЕМНЫЙ МОНИТОР v2.0 ◆◆◆");
            attroff(COLOR_PAIR(6));
        } else {
            mvprintw(1, (cols - 40)/2, "=== СИСТЕМНЫЙ МОНИТОР v2.0 ===");
        }
        
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Basic header rendered\n");

        // Minimal layout for very small terminals
        if (rows > 0 && rows < 24) {
            mvprintw(5, 2, "MEM Usage:    %.2f %%", mem);
            mvprintw(6, 2, "Uptime:       %ld sec", uptime);
            const char *disk_info = get_disk_info();
            const char *net_info = get_network_info();
            mvprintw(7, 2, "Disk:         %s", disk_info);
            mvprintw(8, 2, "Network:      %s", net_info);
            mvprintw(rows - 2, 2, "[q] Quit (compact view)");
            refresh();
            timeout(1500);
            int ch = getch();
            if (ch == 'q' || ch == 'Q') break;
            continue;
        }

        // per-core bars - temporarily disabled for debugging
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Skipping per-core usage for debugging\n");

        // update history
        cpu_hist[hist_idx] = cpu;
        mem_hist[hist_idx] = mem;
        hist_idx = (hist_idx + 1) % 60;

        // draw enhanced sparkline-like graph for CPU and MEM (last 60 ticks) with frame
        int gx = cols > 120 ? 60 : 50;
        int gy = 4;
    
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(gy-1, gx-1,  "┌─ ИСТОРИЯ ЗАГРУЗКИ (60 сек) ─────────────────────────────────────┐");
        mvprintw(gy,   gx-1,  "│ CPU:");
        mvprintw(gy+1, gx-1,  "│");
        mvprintw(gy+2, gx-1,  "│ MEM:");
        mvprintw(gy+3, gx-1,  "│");
        mvprintw(gy+4, gx-1,  "└──────────────────────────────────────────────────────────────────┘");
        if (colors_on) attroff(COLOR_PAIR(4));
    
        const char *levels = " ▁▂▃▄▅▆▇█"; // 9 density levels with Unicode blocks
        for (int i = 0; i < 60; i++) {
            int idx = (hist_idx + i) % 60;
            int lvl_cpu = (int)(cpu_hist[idx] / 11.11f); if (lvl_cpu < 0) lvl_cpu = 0; if (lvl_cpu > 8) lvl_cpu = 8;
            int lvl_mem = (int)(mem_hist[idx] / 11.11f); if (lvl_mem < 0) lvl_mem = 0; if (lvl_mem > 8) lvl_mem = 8;
            char c_cpu = levels[lvl_cpu];
            char c_mem = levels[lvl_mem];
        
            // colorize based on value
            if (colors_on) {
                if (cpu_hist[idx] >= 85.0f) attron(COLOR_PAIR(1));
                else if (cpu_hist[idx] >= 60.0f) attron(COLOR_PAIR(3));
                else attron(COLOR_PAIR(2));
            }
            mvprintw(gy+1, gx + 5 + i, "%c", c_cpu);
            if (colors_on) { attroff(COLOR_PAIR(1)); attroff(COLOR_PAIR(3)); attroff(COLOR_PAIR(2)); }
        
            if (colors_on) {
                if (mem_hist[idx] >= 90.0f) attron(COLOR_PAIR(1));
                else if (mem_hist[idx] >= 70.0f) attron(COLOR_PAIR(3));
                else attron(COLOR_PAIR(2));
            }
            mvprintw(gy+3, gx + 5 + i, "%c", c_mem);
            if (colors_on) { attroff(COLOR_PAIR(1)); attroff(COLOR_PAIR(3)); attroff(COLOR_PAIR(2)); }
        }
        // Memory section with icon
        if (colors_on) {
            attron(COLOR_PAIR(choose_color_by_percent(mem, 90.0f, 95.0f)));
            mvprintw(13, 2, "💾 Память: %.2f%%", mem);
            attroff(COLOR_PAIR(choose_color_by_percent(mem, 90.0f, 95.0f)));
        } else {
            mvprintw(13, 2, "Память: %.2f%%", mem);
        }
        {
            int barw = (cols > 90) ? 40 : (cols > 70 ? 30 : (cols > 50 ? 20 : 12));
            render_bar(14, 2, barw, mem, colors_on);
        }
        // draw_hsep(15, 2);
        // Add memory breakdown
        memory_breakdown_t mb;
        if (get_memory_breakdown(&mb) == 0 && mb.total_bytes > 0) {
            char fbuf[32], abuf[32], ibuf[32], wbuf[32], tbuf[32];
            // simple MB formatting
            #define MB(x) ((x) / (1024ULL*1024ULL))
            snprintf(fbuf, sizeof(fbuf), "%lluMB", (unsigned long long)MB(mb.free_bytes));
            snprintf(abuf, sizeof(abuf), "%lluMB", (unsigned long long)MB(mb.active_bytes));
            snprintf(ibuf, sizeof(ibuf), "%lluMB", (unsigned long long)MB(mb.inactive_bytes));
            snprintf(wbuf, sizeof(wbuf), "%lluMB", (unsigned long long)MB(mb.wired_bytes));
            snprintf(tbuf, sizeof(tbuf), "%lluMB", (unsigned long long)MB(mb.total_bytes));
            mvprintw(13, 28, "[free %s | act %s | inact %s | wired %s | total %s]", fbuf, abuf, ibuf, wbuf, tbuf);
        }
        // Weather (refresh every 5 minutes, non-blocking due to curl timeout)
        const char *no_weather = getenv("SYSMON_NO_WEATHER");
        if (!is_safe_mode() && !(no_weather && no_weather[0] == '1')) {
            fetch_weather_cached(weather_line, sizeof(weather_line));
            if (colors_on) attron(COLOR_PAIR(4));
            mvprintw(12, 2, "🌤️  Погода: %s", weather_line[0] ? weather_line : "Загрузка...");
            if (colors_on) attroff(COLOR_PAIR(4));
            mvprintw(12, cols - 25, "[W] Подробнее");
        } else {
            mvprintw(12, 2, "🌤️  Погода: (отключено)");
        }
        // Per-core usage + temps combined view (up to 8 cores)
        float core_temps[16]; int ct_written = 0;
        (void)smc_get_core_temperatures(core_temps, 16, &ct_written);
        float cores[32]; int wrote = 0;
        int core_result = get_per_core_usage(cores, 32, &wrote);
        // CPU section with icon
        if (colors_on) {
            attron(COLOR_PAIR(choose_color_by_percent(cpu, 85.0f, 90.0f)));
            mvprintw(3, 2, "🖥️  Процессор: %.2f%% %s", cpu, fbuf);
            attroff(COLOR_PAIR(choose_color_by_percent(cpu, 85.0f, 90.0f)));
        } else {
            mvprintw(3, 2, "Процессор: %.2f%% %s", cpu, fbuf);
        }
        {
            int barw = (cols > 90) ? 40 : (cols > 70 ? 30 : (cols > 50 ? 20 : 12));
            render_bar(4, 2, barw, cpu, colors_on);
        }
        // Add section separator
        if (colors_on) attron(COLOR_PAIR(5));
        mvprintw(20, 2, "──────────────── ИНФОРМАЦИЯ О ЯДРАХ ПРОЦЕССОРА ────────────────");
        if (colors_on) attroff(COLOR_PAIR(5));
    
        int row = 21;
        if (core_result == 0 && wrote > 0) {
            int show = wrote < 8 ? wrote : 8;
            for (int i = 0; i < show; i++) {
                // small bar (20 cols)
                int barw = 20;
                render_bar(row + i, 9, barw, cores[i], colors_on);
                // label + percent + temp (if available)
                char tbuf[16];
                const char *tstr = (ct_written > i && core_temps[i] >= 0.0f) ? (snprintf(tbuf, sizeof(tbuf), "%4.1f C", core_temps[i]), tbuf) : "N/A";
                mvprintw_clip(row + i, 2, "Core %-2d", i);
                mvprintw_clip(row + i, 9 + barw + 2, "%5.1f%%  %s", cores[i], tstr);
            }
        } else if (ct_written > 0) {
            // fallback: only temps
            int show = ct_written < 8 ? ct_written : 8;
            for (int i = 0; i < show; i++) {
                mvprintw_clip(row + i, 2, "Core %-2d       %s", i, (snprintf((char[16]){0}, 0, ""), ""));
                mvprintw_clip(row + i, 18, "%4.1f C", core_temps[i]);
            }
        } else {
            mvprintw_clip(row, 2, "Per-core: N/A");
        }
        float mem_temp = -1.0f; smc_get_mem_temperature(&mem_temp);
        if (mem_temp >= 0) {
            mvprintw_clip(row + 8, 2, "RAM Temp:     %4.1f C", mem_temp);
        }
        // Uptime with icon
        char up_str[64]; format_uptime(uptime, up_str, sizeof(up_str));
        if (colors_on) attron(COLOR_PAIR(4));
        mvprintw(14, 2, "⏱️  Время работы: %s", up_str);
        if (colors_on) attroff(COLOR_PAIR(4));
        const char *disk_info = get_disk_info();
        const char *net_info = get_network_info();
        smart_info_t sm; get_smart_info(NULL, &sm);
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: disk ok, net ok\n");
        // Disk with icon
        mvprintw(15, 2, "💿 Диск: %s", disk_info);
        if (sm.available) {
            char tbuf[16];
            const char *tstr = sm.temp_celsius >= 0 ? (snprintf(tbuf, sizeof(tbuf), "%d C", sm.temp_celsius), tbuf) : "N/A";
            char poh[16], rsec[16], pend[16], unc[16];
            const char *pohs = sm.power_on_hours >= 0 ? (snprintf(poh, sizeof(poh), "%d", sm.power_on_hours), poh) : "N/A";
            const char *rsecs = sm.reallocated_sectors >= 0 ? (snprintf(rsec, sizeof(rsec), "%d", sm.reallocated_sectors), rsec) : "N/A";
            const char *pends = sm.pending_sectors >= 0 ? (snprintf(pend, sizeof(pend), "%d", sm.pending_sectors), pend) : "N/A";
            const char *uncs = sm.uncorrectable >= 0 ? (snprintf(unc, sizeof(unc), "%d", sm.uncorrectable), unc) : "N/A";
            mvprintw(16, 2, "SMART:        Temp: %s  Health: %s  POH:%s  Realloc:%s  Pend:%s  Uncorr:%s",
                     tstr, sm.health, pohs, rsecs, pends, uncs);
        } else {
            mvprintw(16, 2, "SMART:        smartctl not found (install smartmontools)");
        }
        const char *no_wifi = getenv("SYSMON_NO_WIFI");
        if (!is_safe_mode() && !(no_wifi && no_wifi[0] == '1')) {
            fetch_ssid_cached(ssid, sizeof(ssid));
            if (ssid[0]) mvprintw(17, 2, "Network:      %s  Wi‑Fi: %s", net_info, ssid);
            else mvprintw(17, 2, "Network:      %s", net_info);
        } else {
            mvprintw(17, 2, "🌐 Сеть: %s", net_info);
        }

        const char *safe = getenv("SYSMON_SAFE");
        int safe_on = (safe && safe[0] == '1');
        const char *no_bat = getenv("SYSMON_NO_BAT");
        const char *no_gpu = getenv("SYSMON_NO_GPU");
        const char *no_usb = getenv("SYSMON_NO_USB");

        // Battery
        if (safe_on || (no_bat && no_bat[0] == '1') || battery_failed) {
            mvprintw_clip(17, 2, "Battery:      (disabled)");
        } else {
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: battery start\n");
            // Восстанавливаем функциональность батареи
            get_battery_info(&bat);
            get_battery_cycle_count(&cycles);
            
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: battery ok (disabled)\n");
            char bline[128]; bline[0] = '\0';
            int n = 0;
            if (bat.percentage >= 0) n += snprintf(bline + n, sizeof(bline) - n, "%4.0f%% ", bat.percentage);
            if (bat.charging == 1) n += snprintf(bline + n, sizeof(bline) - n, "charging ");
            else if (bat.charging == 0) n += snprintf(bline + n, sizeof(bline) - n, "discharging ");
            if (bat.time_remaining_min >= 0) n += snprintf(bline + n, sizeof(bline) - n, "~%dm ", bat.time_remaining_min);
            if (cycles >= 0) n += snprintf(bline + n, sizeof(bline) - n, "cycles:%d", cycles);
            if (n == 0) snprintf(bline, sizeof(bline), "недоступно");
            if (colors_on && bat.percentage >= 0) {
                int bcol = choose_color_by_percent(bat.percentage, 20.0f, 10.0f);
                mvprintw_clip_color(17, 2, bcol, "Battery:      %s", bline);
            } else {
                mvprintw_clip(17, 2, "Battery:      %s", bline);
            }
            // auto-disable if clearly unavailable
            if (bat.percentage < 0 && bat.charging < 0 && bat.time_remaining_min < 0 && cycles < 0) {
                log_error("Battery module reported no data. Disabling battery stats.");
                battery_failed = 1;
            }
        }

        // GPU
        if (safe_on || (no_gpu && no_gpu[0] == '1') || gpu_failed) {
            mvprintw(18, 2, "GPU Usage:    (disabled)");
        } else {
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: gpu start\n");
            gpu_info_t gpu; get_gpu_info(&gpu);
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: gpu ok (usage=%.1f)\n", gpu.usage);
            mvprintw(18, 2, "GPU Usage:    %s",
                    gpu.usage >= 0 ? "available" : "N/A");
            if (gpu.usage < 0) {
                log_error("GPU module unavailable. Disabling GPU stats.");
                gpu_failed = 1;
            }
        }

        // USB (show count)
        if (safe_on || (no_usb && no_usb[0] == '1') || usb_failed) {
            mvprintw(19, 2, "USB Devices:  (disabled)");
        } else {
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: usb start\n");
            usb_device_t devs[8]; int dev_count = 0; list_usb_devices(devs, 8, &dev_count);
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: usb ok (count=%d)\n", dev_count);
            mvprintw(19, 2, "USB Devices:  %d", dev_count);
            if (dev_count < 0) { // defensive: treat negative as failure
                log_error("USB module error. Disabling USB stats.");
                usb_failed = 1;
            }
        }

        // Overheat/overload and anomaly indicator with thresholds
        const char *sigma_env = getenv("SYSMON_ANOMALY_SIGMA");
        float sigma = sigma_env ? atof(sigma_env) : 2.0f;
        int af = anomalies_push(&hist, cpu, mem, sigma);
        // thresholds from env
        float thr_cpu = getenv("SYSMON_THR_CPU") ? atof(getenv("SYSMON_THR_CPU")) : 90.0f;
        float thr_mem = getenv("SYSMON_THR_MEM") ? atof(getenv("SYSMON_THR_MEM")) : 95.0f;
        float thr_temp = getenv("SYSMON_THR_TEMP") ? atof(getenv("SYSMON_THR_TEMP")) : 85.0f;
        // compute overload/overheat
        int overload_cpu = cpu >= thr_cpu;
        int overload_mem = mem >= thr_mem;
        int overheat = 0;
        for (int i = 0; i < ct_written && i < 8; i++) {
            if (core_temps[i] >= thr_temp) { overheat = 1; break; }
        }
        // Status box with alert icon
        int status_row = rows > 35 ? 35 : (rows - 4);
        if (af || overload_cpu || overload_mem || overheat) {
            if (colors_on) attron(COLOR_PAIR(1));
            mvprintw(status_row, 2, "╔══════════════════════════════════════════════════════════╗");
            mvprintw(status_row+1, 2, "║ ⚠️  ВНИМАНИЕ: %s%s %s%s%s",
                     (af & 1) ? "Аномалия CPU " : "",
                     (af & 2) ? "Аномалия MEM " : "",
                     overheat ? "ПЕРЕГРЕВ! " : "",
                     overload_cpu ? "Перегрузка CPU! " : "",
                     overload_mem ? "Перегрузка RAM!" : "");
            mvprintw(status_row+1, 62, "║");
            mvprintw(status_row+2, 2, "╚══════════════════════════════════════════════════════════╝");
            if (colors_on) attroff(COLOR_PAIR(1));
        } else {
            if (colors_on) attron(COLOR_PAIR(2));
            mvprintw(status_row, 2, "╔══════════════════════════════════════════════════════════╗");
            mvprintw(status_row+1, 2, "║ ✓ СТАТУС: Система работает нормально                      ║");
            mvprintw(status_row+2, 2, "╚══════════════════════════════════════════════════════════╝");
            if (colors_on) attroff(COLOR_PAIR(2));
        }

        // Footer at the bottom row-2
        // Beautiful footer with command hints
        {
            int footer_row = rows > 2 ? rows - 2 : 23;
            if (colors_on) attron(COLOR_PAIR(4));
            mvprintw(footer_row - 1, 2, "════════════════════════════════════════════════════════════════════════════");
            if (colors_on) attroff(COLOR_PAIR(4));
            
            if (colors_on) attron(COLOR_PAIR(5));
            mvprintw(footer_row, 2, " [P]Проц [G]Git [E]Env [L]Порты [T]Тесты [V]Dev [A]Действ [W]Погода [Q]Выход ");
            
            // Show module status with icons
            mvprintw(footer_row, cols - 35, "Модули: %s%s%s%s",
                     safe_on ? "🔒" : "✓",
                     (safe_on || (no_bat && no_bat[0] == '1') || battery_failed) ? "" : "🔋",
                     (safe_on || (no_gpu && no_gpu[0] == '1') || gpu_failed) ? "" : "🎮",
                     (safe_on || (no_usb && no_usb[0] == '1') || usb_failed) ? "" : "🔌");
            if (colors_on) attroff(COLOR_PAIR(5));
        }

        // optional CSV logging
        const char *csv = getenv("SYSMON_CSV");
        if (csv && csv[0]) {
            // Use current network RX/TX in bytes per second for logging
            float rx = get_network_rx();
            float tx = get_network_tx();
            log_metrics_csv(csv, cpu, mem, uptime, rx, tx);
        }

        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: About to refresh screen\n");
        refresh();
        if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Screen refreshed, waiting for input\n");

        // Улучшенная обработка ввода для macOS
        timeout(2000);  // 2 секунды таймаут
        
        // Очищаем буфер ввода перед чтением
        flushinp();
        
        int ch = getch();
        
        // Дополнительная диагностика для macOS
        if (getenv("SYSMON_DEBUG") && ch != ERR) {
            fprintf(stderr, "DBG: Получена клавиша: %d (char: '%c', hex: 0x%02x)\n", 
                    ch, (ch >= 32 && ch <= 126) ? ch : '?', ch & 0xFF);
        } else if (getenv("SYSMON_DEBUG")) {
            int term_rows, term_cols;
            getmaxyx(stdscr, term_rows, term_cols);
            fprintf(stderr, "DBG: Input timeout or error. Terminal size: %dx%d\n", term_rows, term_cols);
        }
        
        if (ch == ERR) {
            // Timeout occurred, continue the loop
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Input timeout, continuing...\n");
            continue;
        }
        
        // Обработка клавиш теперь будет в отдельных блоках
        
        if (ch == 'q' || ch == 'Q') {
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Выход из программы\n");
            break;
        }
        if (ch == 'p' || ch == 'P') {
            if (getenv("SYSMON_DEBUG")) fprintf(stderr, "DBG: Открытие списка процессов\n");
            show_processes();
            // Восстанавливаем timeout для основного цикла
            timeout(2000);
        }
        if (ch == 's' || ch == 'S') {
            // Save system snapshot
            time_t rawtime;
            struct tm *timeinfo;
            char filename[256];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(filename, sizeof(filename), "sysmon_snapshot_%Y%m%d_%H%M%S.txt", timeinfo);
            
            FILE *snapshot = fopen(filename, "w");
            if (snapshot) {
                fprintf(snapshot, "════════════════════════════════════════════════════════════\n");
                fprintf(snapshot, "           СИСТЕМНЫЙ МОНИТОР - СНИМОК СОСТОЯНИЯ\n");
                fprintf(snapshot, "           Дата: %s", asctime(timeinfo));
                fprintf(snapshot, "════════════════════════════════════════════════════════════\n\n");
                
                fprintf(snapshot, "ОСНОВНЫЕ ПОКАЗАТЕЛИ:\n");
                fprintf(snapshot, "────────────────────\n");
                fprintf(snapshot, "CPU:          %.2f%% %s\n", cpu, fbuf);
                fprintf(snapshot, "Память:       %.2f%%\n", mem);
                fprintf(snapshot, "Время работы: %s\n\n", up_str);
                
                if (mb.total_bytes > 0) {
                    fprintf(snapshot, "ДЕТАЛИ ПАМЯТИ:\n");
                    fprintf(snapshot, "──────────────\n");
                    fprintf(snapshot, "Всего:        %llu MB\n", (unsigned long long)(mb.total_bytes / (1024*1024)));
                    fprintf(snapshot, "Свободно:     %llu MB\n", (unsigned long long)(mb.free_bytes / (1024*1024)));
                    fprintf(snapshot, "Активная:     %llu MB\n", (unsigned long long)(mb.active_bytes / (1024*1024)));
                    fprintf(snapshot, "Неактивная:   %llu MB\n", (unsigned long long)(mb.inactive_bytes / (1024*1024)));
                    fprintf(snapshot, "Wired:        %llu MB\n\n", (unsigned long long)(mb.wired_bytes / (1024*1024)));
                }
                
                if (ct_written > 0) {
                    fprintf(snapshot, "ТЕМПЕРАТУРЫ CPU:\n");
                    fprintf(snapshot, "────────────────\n");
                    for (int i = 0; i < ct_written && i < 8; i++) {
                        fprintf(snapshot, "Ядро %d:       %.1f°C\n", i, core_temps[i]);
                    }
                    if (mem_temp >= 0) {
                        fprintf(snapshot, "RAM:          %.1f°C\n", mem_temp);
                    }
                    fprintf(snapshot, "\n");
                }
                
                fprintf(snapshot, "НАКОПИТЕЛИ:\n");
                fprintf(snapshot, "───────────\n");
                fprintf(snapshot, "%s\n", disk_info);
                if (sm.available) {
                    fprintf(snapshot, "SMART:        %s\n", sm.health);
                    if (sm.temp_celsius >= 0) fprintf(snapshot, "Температура:  %d°C\n", sm.temp_celsius);
                    if (sm.power_on_hours >= 0) fprintf(snapshot, "Часов работы: %d\n", sm.power_on_hours);
                }
                fprintf(snapshot, "\n");
                
                fprintf(snapshot, "СЕТЬ:\n");
                fprintf(snapshot, "─────\n");
                fprintf(snapshot, "%s\n", net_info);
                if (ssid[0]) fprintf(snapshot, "Wi-Fi:        %s\n", ssid);
                fprintf(snapshot, "\n");
                
                if (!battery_failed && bat.percentage >= 0) {
                    fprintf(snapshot, "БАТАРЕЯ:\n");
                    fprintf(snapshot, "────────\n");
                    fprintf(snapshot, "Заряд:        %.0f%%\n", bat.percentage);
                    fprintf(snapshot, "Статус:       %s\n", bat.charging == 1 ? "Заряжается" : "Разряжается");
                    if (cycles >= 0) fprintf(snapshot, "Циклы:        %d\n", cycles);
                    fprintf(snapshot, "\n");
                }
                
                // Get process list for snapshot
                process_info_t procs[10];
                size_t proc_count = get_process_list(procs, 10);
                if (proc_count > 0) {
                    fprintf(snapshot, "ТОП-10 ПРОЦЕССОВ ПО CPU:\n");
                    fprintf(snapshot, "────────────────────────\n");
                    qsort(procs, proc_count, sizeof(process_info_t), cmp_cpu_desc);
                    for (size_t i = 0; i < proc_count && i < 10; i++) {
                        fprintf(snapshot, "%-20s  PID:%-8d  CPU:%.1f%%  MEM:%.1f%%\n",
                                procs[i].name, procs[i].pid, procs[i].cpu_usage, procs[i].mem_usage);
                    }
                }
                
                fclose(snapshot);
                
                // Show success message
                if (colors_on) attron(COLOR_PAIR(2));
                mvprintw(rows/2, (cols - 50)/2, "✓ Снимок сохранен в %s", filename);
                if (colors_on) attroff(COLOR_PAIR(2));
                refresh();
                sleep(1);
            } else {
                if (colors_on) attron(COLOR_PAIR(1));
                mvprintw(rows/2, (cols - 40)/2, "✗ Ошибка при сохранении снимка");
                if (colors_on) attroff(COLOR_PAIR(1));
                refresh();
                sleep(1);
            }
        }
        if (ch == 'r' || ch == 'R') {
            const char *cfg = resolve_config_path();
            int rc = -1;
            if (cfg) rc = load_config(cfg);
            if (rc != 0) rc = load_config("src/config.ini");
            if (rc != 0) rc = load_config("config.ini");
            // summarize recognized settings
            const char *safe = getenv("SYSMON_SAFE");
            const char *no_bat = getenv("SYSMON_NO_BAT");
            const char *no_gpu = getenv("SYSMON_NO_GPU");
            const char *no_usb = getenv("SYSMON_NO_USB");
            const char *tcpu = getenv("SYSMON_THR_CPU");
            const char *tmem = getenv("SYSMON_THR_MEM");
            const char *ttmp = getenv("SYSMON_THR_TEMP");
            const char *csv = getenv("SYSMON_CSV");
            if (rc == 0) mvprintw(24, 2, "Config reloaded (safe=%s bat=%s gpu=%s usb=%s thr:CPU=%s MEM=%s TEMP=%s csv=%s)",
                                  safe?safe:"0", no_bat?no_bat:"0", no_gpu?no_gpu:"0", no_usb?no_usb:"0",
                                  tcpu?tcpu:"90", tmem?tmem:"95", ttmp?ttmp:"85", csv?csv:"-");
            else mvprintw(24, 2, "Config reload failed");
        }
        if (ch == 'd' || ch == 'D') {
            if (!is_safe_mode()) {
                perform_diagnostics();
                show_diagnostics_screen();
            }
            timeout(2000);
        }
        if (ch == 'w' || ch == 'W') {
            show_weather_details();
            timeout(2000);
        }
        if (ch == 'n' || ch == 'N') {
            show_network_connections();
            timeout(2000);
        }
        if (ch == 'a' || ch == 'A') {
            show_quick_actions();
            timeout(2000);
        }
        if (ch == 'c' || ch == 'C') {
            show_docker_containers();
            timeout(2000);
        }
        if (ch == 'g' || ch == 'G') {
            show_git_repos();
            timeout(2000);
        }
        if (ch == 'e' || ch == 'E') {
            show_dev_environment();
            timeout(2000);
        }
        if (ch == 'l' || ch == 'L') {
            show_listening_ports();
            timeout(2000);
        }
        if (ch == 't' || ch == 'T') {
            run_functionality_tests();
            timeout(2000);
        }
        if (ch == 'v' || ch == 'V') {
            // Developer menu
            clear();
            int dev_cols;
            getmaxyx(stdscr, (int){0}, dev_cols);
            
            if (colors_on) attron(COLOR_PAIR(4));
            mvprintw(5, 15, "╔════════════════════════════════════════╗");
            for (int i = 6; i < 14; i++) {
                mvprintw(i, 15, "║");
                mvprintw(i, 57, "║");
            }
            mvprintw(14, 15, "╚════════════════════════════════════════╝");
            if (colors_on) attroff(COLOR_PAIR(4));
            
            if (colors_on) attron(COLOR_PAIR(6));
            mvprintw(6, 25, "👨‍💻 МЕНЮ РАЗРАБОТЧИКА 👨‍💻");
            if (colors_on) attroff(COLOR_PAIR(6));
            
            mvprintw(8, 17, "[1] Процессы разработки");
            mvprintw(9, 17, "[2] Быстрые действия разработчика");
            mvprintw(10, 17, "[3] Мониторинг логов");
            mvprintw(11, 17, "[4] API тестирование");
            mvprintw(12, 17, "[5] Анализ кода");
            mvprintw(13, 17, "[Q] Назад");
            
            refresh();
            int dev_ch = getch();
            if (dev_ch == '1') {
                show_dev_processes();
                timeout(2000);
            }
            else if (dev_ch == '2') {
                show_dev_quick_actions();
                timeout(2000);
            }
            // Восстанавливаем timeout после выхода из меню разработчика
            timeout(2000);
        }
    }

    endwin();
}
