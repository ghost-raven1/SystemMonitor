#include "ui/ui_dev_tools.h"
#include "ui/ui.h"
#include "ui/ui_helpers.h"
#include "core/developer.h"
#include <ncurses.h>
#include <string.h>

// Show listening ports
void show_listening_ports(void) {
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
    mvprintw(rows-2, 2, "╚════════════════════════════════════════════════════════════════════╝");
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

// Show git repositories
void show_git_repos(void) {
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

// Show development environment
void show_dev_environment(void) {
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

// Show development processes
void show_dev_processes(void) {
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

// Show quick actions for developers
void show_dev_quick_actions(void) {
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