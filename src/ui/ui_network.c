#include "ui/ui_network.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include <ncurses.h>
#include <string.h>
#include <unistd.h>

// Enhanced weather display
void show_weather_details(void) {
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
void show_network_connections(void) {
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
void show_docker_containers(void) {
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