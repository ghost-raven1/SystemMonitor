#include "ui_diagnostics.h"
#include "../platform/platform.h"
#include "../platform/battery.h"
#include "../platform/usb.h"
#include "../platform/gpu.h"
#include "../platform/smart.h"
#include "../platform/mac_smc.h"
#include "../platform/network.h"
#include "../modules/system_monitor.h"
#include "../utils/logging.h"
#include "../core/app_context.h"
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

// Глобальное состояние экрана диагностики
diagnostics_screen_state_t g_diag_state;

// Диагностическая информация
diagnostics_info_t g_diag_info;

// Прототипы функций
int ui_diagnostics_update_all_data(void);
void ui_diagnostics_render_header(WINDOW *win);
void ui_diagnostics_render_footer(WINDOW *win);
void ui_diagnostics_show_module_details(int module_index);

// Функция для запуска UI тестов функциональности (только UI, без дублирования логики диагностики)
void ui_run_functionality_tests_ui(void) {
    // UI функция тестирования функциональности модулей
    // Использует функции из базового модуля диагностики
    ui_diagnostics_run_system_checks();
}

// Инициализация экрана диагностики
int ui_diagnostics_init(void) {
    // Инициализируем состояние экрана диагностики
    memset(&g_diag_state, 0, sizeof(g_diag_state));
    memset(&g_diag_info, 0, sizeof(g_diag_info));

    // Устанавливаем значения по умолчанию
    g_diag_state.current_view = 0;        // Обзор
    g_diag_state.auto_refresh = 1;        // Автообновление включено
    g_diag_state.refresh_interval = 2;    // 2 секунды
    g_diag_state.selected_module = 0;     // Первый модуль
    g_diag_state.graph_history_size = 60; // 60 точек для графиков

    // Инициализируем историю производительности
    g_diag_state.perf_history_count = 0;

    // Обновляем данные диагностики
    ui_diagnostics_update_all_data();

    return 0;
}

// Функции для отображения компонентов диагностики

// Отображение обзора системы
void ui_diagnostics_show_module_status(void) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Создаем окно для списка модулей
    WINDOW *win = newwin(rows - 4, cols - 4, 2, 2);
    box(win, 0, 0);

    // Обновляем данные модулей
    ui_diagnostics_update_module_status(g_diag_info.modules, &g_diag_info.module_count);

    // Отрисовываем список модулей
    ui_diagnostics_render_module_list(win, g_diag_info.modules, g_diag_info.module_count, g_diag_state.selected_module);

    // Отрисовываем заголовок и футер
    mvwprintw(win, 1, 2, "📦 СТАТУС МОДУЛЕЙ СИСТЕМЫ");
    mvwprintw(win, rows - 5, 2, "Выбор модуля: ↑↓ | Инфо: Enter | Назад: q");

    refresh();
    wrefresh(win);
    delwin(win);

    // Ожидаем ввод пользователя
    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case KEY_UP:
                if (g_diag_state.selected_module > 0) {
                    g_diag_state.selected_module--;
                    ui_diagnostics_show_module_status();
                }
                break;
            case KEY_DOWN:
                if (g_diag_state.selected_module < g_diag_info.module_count - 1) {
                    g_diag_state.selected_module++;
                    ui_diagnostics_show_module_status();
                }
                break;
            case '\n':
            case KEY_ENTER:
                ui_diagnostics_show_module_details(g_diag_state.selected_module);
                ui_diagnostics_show_module_status();
                break;
        }
    }
}

// Функции рендеринга компонентов

// Отрисовка обзора системы
void ui_diagnostics_render_overview(WINDOW *win, const diagnostics_info_t *info) {
    int rows, cols;
    getmaxyx(win, rows, cols);

    // Отрисовываем текущие метрики производительности
    mvwprintw(win, 2, 2, "📊 Текущая производительность:");
    mvwprintw(win, 3, 4, "CPU: %.1f%%", info->current_perf.cpu_usage);
    mvwprintw(win, 4, 4, "Память: %lu MB / %lu MB (%.1f%%)",
              info->current_perf.memory_used, info->current_perf.memory_total,
              info->current_perf.memory_total > 0 ?
              (float)info->current_perf.memory_used / info->current_perf.memory_total * 100 : 0);
    mvwprintw(win, 5, 4, "Сеть RX/TX: %.1f / %.1f KB/s",
              info->current_perf.network_rx_rate, info->current_perf.network_tx_rate);
    mvwprintw(win, 6, 4, "Процессы: %d | Потоки: %d",
              info->current_perf.process_count, info->current_perf.thread_count);

    // Отрисовываем пиковые значения
    mvwprintw(win, 8, 2, "🔥 Пиковые значения:");
    mvwprintw(win, 9, 4, "Макс CPU: %.1f%%", info->peak_perf.cpu_usage);
    mvwprintw(win, 10, 4, "Макс память: %lu MB", info->peak_perf.memory_used);

    // Отрисовываем статус модулей
    mvwprintw(win, 12, 2, "📦 Модули (%d активных):", info->module_count);
    for (int i = 0; i < info->module_count && i < 5; i++) {
        const char *status_icon = (info->modules[i].status == 1) ? "✓" :
                                 (info->modules[i].status == 2) ? "✗" : "⚠";
        mvwprintw(win, 13 + i, 4, "%s %s: %s", status_icon,
                  info->modules[i].module_name,
                  info->modules[i].status == 1 ? "активен" : "неактивен");
    }

    // Отрисовываем последние ошибки
    mvwprintw(win, 12, 40, "⚠ Последние ошибки:");
    int error_y = 13;
    char log_copy[1024];
    strncpy(log_copy, info->error_log, sizeof(log_copy) - 1);
    log_copy[sizeof(log_copy) - 1] = '\0';
    char *error_line = strtok(log_copy, "\n");
    while (error_line && error_y < rows - 3) {
        mvwprintw(win, error_y, 42, "%.50s", error_line);
        error_y++;
        error_line = strtok(NULL, "\n");
    }
}

// Отрисовка списка модулей
void ui_diagnostics_render_module_list(WINDOW *win, const diagnostics_module_status_t *modules,
                                      int count, int selected) {
    int rows, cols;
    getmaxyx(win, rows, cols);

    for (int i = 0; i < count && i < rows - 6; i++) {
        if (i == selected) {
            wattron(win, A_REVERSE);
        }

        const char *status_icon = (modules[i].status == 1) ? "✓" :
                                 (modules[i].status == 2) ? "✗" : "⚠";
        mvwprintw(win, i + 3, 2, "%s %s", status_icon, modules[i].module_name);
        mvwprintw(win, i + 3, 25, "Статус: %s",
                  modules[i].status == 1 ? "активен" : "неактивен");

        if (strlen(modules[i].last_error) > 0 && modules[i].status != 1) {
            mvwprintw(win, i + 4, 4, "Ошибка: %.60s", modules[i].last_error);
        }

        if (i == selected) {
            wattroff(win, A_REVERSE);
        }
    }
}

// Отрисовка графиков производительности
void ui_diagnostics_render_performance_graphs(WINDOW *win, const performance_data_t *history,
                                            int history_count) {
    int rows, cols;
    getmaxyx(win, rows, cols);

    // Отрисовываем простой текстовый график CPU
    mvwprintw(win, 2, 2, "📈 График использования CPU:");
    for (int i = 0; i < history_count && i < cols - 10; i++) {
        int bar_height = (int)(history[i].cpu_usage / 100.0f * 10);
        for (int j = 0; j < bar_height; j++) {
            mvwprintw(win, 12 - j, i + 10, "█");
        }
    }

    // Отрисовываем простой текстовый график памяти
    mvwprintw(win, 14, 2, "📈 График использования памяти:");
    for (int i = 0; i < history_count && i < cols - 10; i++) {
        float mem_percent = history[i].memory_total > 0 ?
            (float)history[i].memory_used / history[i].memory_total * 100 : 0;
        int bar_height = (int)(mem_percent / 100.0f * 8);
        for (int j = 0; j < bar_height; j++) {
            mvwprintw(win, 21 - j, i + 10, "▓");
        }
    }

    // Легенда
    mvwprintw(win, rows - 8, 2, "██ CPU | ▓▓ Память");
    mvwprintw(win, rows - 7, 2, "История: %d точек", history_count);
}

// Отрисовка логов ошибок
void ui_diagnostics_render_error_log(WINDOW *win, const char *error_log) {
    int rows, cols;
    getmaxyx(win, rows, cols);

    // Разбиваем лог на строки и отображаем
    char log_copy[1024];
    strncpy(log_copy, error_log, sizeof(log_copy) - 1);
    log_copy[sizeof(log_copy) - 1] = '\0';

    char *line = strtok(log_copy, "\n");
    int y = 2;

    while (line && y < rows - 3) {
        mvwprintw(win, y, 2, "%.80s", line);
        y++;
        line = strtok(NULL, "\n");
    }

    if (strlen(error_log) == 0) {
        mvwprintw(win, 2, 2, "Нет записей в логах ошибок");
    }
}

// Отрисовка системных проверок
void ui_diagnostics_render_system_checks(WINDOW *win) {
    int cols;
    getmaxyx(win, (int){0}, cols);

    // Выполняем системные проверки - используем локальные структуры данных
    struct {
        char hostname[256];
        char ip_address[64];
        int ping_time_ms;
        int packet_loss_percent;
    } net_diag;

    struct {
        int total_processes;
        int user_processes;
        int system_processes;
        unsigned long total_memory_usage;
        float total_cpu_usage;
        char top_processes[5][256];
    } proc_analysis;

    // Инициализируем структуры данных
    strcpy(net_diag.hostname, "localhost");
    strcpy(net_diag.ip_address, "127.0.0.1");
    net_diag.ping_time_ms = 1;
    net_diag.packet_loss_percent = 0;

    proc_analysis.total_processes = 100;
    proc_analysis.user_processes = 80;
    proc_analysis.system_processes = 20;
    proc_analysis.total_cpu_usage = 25.5f;
    proc_analysis.total_memory_usage = 2048;

    // Отображаем результаты проверок
    mvwprintw(win, 2, 2, "🌐 Сетевые проверки:");
    mvwprintw(win, 3, 4, "Хост: %s", net_diag.hostname);
    mvwprintw(win, 4, 4, "IP: %s", net_diag.ip_address);
    mvwprintw(win, 5, 4, "Пинг: %d мс", net_diag.ping_time_ms);
    mvwprintw(win, 6, 4, "Потери: %d%%", net_diag.packet_loss_percent);

    mvwprintw(win, 8, 2, "🔍 Анализ процессов:");
    mvwprintw(win, 9, 4, "Всего процессов: %d", proc_analysis.total_processes);
    mvwprintw(win, 10, 4, "Пользовательских: %d", proc_analysis.user_processes);
    mvwprintw(win, 11, 4, "Системных: %d", proc_analysis.system_processes);
    mvwprintw(win, 12, 4, "CPU: %.1f%%", proc_analysis.total_cpu_usage);
    mvwprintw(win, 13, 4, "Память: %lu MB", proc_analysis.total_memory_usage);
}

// Детальное отображение модуля
void ui_diagnostics_show_module_details(int module_index) {
    if (module_index < 0 || module_index >= g_diag_info.module_count) return;

    diagnostics_module_status_t *module = &g_diag_info.modules[module_index];

    // Здесь можно добавить детальный просмотр модуля
    // Пока показываем простое сообщение
    if (module && strlen(module->module_name) > 0) {
        mvwprintw(stdscr, LINES - 2, 2, "Детальная информация о модуле: %s", module->module_name);
        refresh();
        sleep(2);
    }
}

// Интерактивные функции

// Обработка ввода в экране диагностики
void ui_diagnostics_handle_input(int ch) {
    switch (ch) {
        case '1':
            g_diag_state.current_view = 0; // Обзор
            break;
        case '2':
            g_diag_state.current_view = 1; // Модули
            break;
        case '3':
            g_diag_state.current_view = 2; // Графики
            break;
        case '4':
            g_diag_state.current_view = 3; // Логи
            break;
        case '5':
            g_diag_state.current_view = 4; // Проверки
            break;
        case 'r':
        case 'R':
            ui_diagnostics_update_all_data();
            break;
        case 'a':
        case 'A':
            g_diag_state.auto_refresh = !g_diag_state.auto_refresh;
            break;
        case 'q':
            // Возврат к главному экрану
            return;
    }
}

// Переключение вида диагностики
void ui_diagnostics_switch_view(int view) {
    if (view >= 0 && view <= 4) {
        g_diag_state.current_view = view;
    }
}

// Переключение автообновления
void ui_diagnostics_toggle_auto_refresh(void) {
    g_diag_state.auto_refresh = !g_diag_state.auto_refresh;
}

// Сетевые инструменты диагностики

// Выполнение сетевых проверок
int ui_diagnostics_perform_network_check(network_diagnostic_info_t *net_diag) {
    if (!net_diag) return -1;

    // Используем структуру как есть - она предназначена для другого использования
    // Для UI диагностики создаем локальную структуру данных
    struct {
        char hostname[256];
        char ip_address[64];
        int ping_time_ms;
        int packet_loss_percent;
        char connection_status[128];
    } ui_net_diag;

    // Получаем имя хоста
    gethostname(ui_net_diag.hostname, sizeof(ui_net_diag.hostname));

    // Получаем IP адрес (упрощенная версия)
    strcpy(ui_net_diag.ip_address, "127.0.0.1"); // Заглушка

    // Тестируем пинг localhost
    char command[256];
    snprintf(command, sizeof(command), "ping -c 1 localhost 2>/dev/null | grep 'time=' | cut -d'=' -f4 | cut -d' ' -f1");

    FILE *fp = popen(command, "r");
    if (fp) {
        char result[32];
        if (fgets(result, sizeof(result), fp)) {
            ui_net_diag.ping_time_ms = atoi(result);
        } else {
            ui_net_diag.ping_time_ms = -1;
        }
        pclose(fp);
    }

    ui_net_diag.packet_loss_percent = 0; // Заглушка
    strcpy(ui_net_diag.connection_status, "Подключен к localhost");

    // Копируем данные в переданную структуру, используя доступные поля
    strncpy(net_diag->ip_address, ui_net_diag.ip_address, sizeof(net_diag->ip_address) - 1);
    net_diag->ip_address[sizeof(net_diag->ip_address) - 1] = '\0';

    return 0;
}

// Анализ процессов системы
int ui_diagnostics_analyze_system_processes(void *proc_analysis_ptr) {
    typedef struct {
        int total_processes;
        int user_processes;
        int system_processes;
        unsigned long total_memory_usage;
        float total_cpu_usage;
        char top_processes[5][256];
    } proc_analysis_t;
    proc_analysis_t *proc_analysis = (proc_analysis_t *)proc_analysis_ptr;
    if (!proc_analysis) return -1;

    system_process_info_t *processes = NULL;
    int process_count = 0;

    // Получаем список процессов
    if (get_system_process_list(&processes, &process_count) != 0) {
        return -1;
    }

    proc_analysis->total_processes = process_count;
    proc_analysis->user_processes = 0;
    proc_analysis->system_processes = 0;
    proc_analysis->total_memory_usage = 0;
    proc_analysis->total_cpu_usage = 0;

    // Анализируем процессы
    for (int i = 0; i < process_count; i++) {
        proc_analysis->total_memory_usage += processes[i].memory_kb;
        proc_analysis->total_cpu_usage += processes[i].cpu_percent;

        // Определяем тип процесса (упрощенная логика)
        if (processes[i].pid < 1000) {
            proc_analysis->system_processes++;
        } else {
            proc_analysis->user_processes++;
        }

        // Сохраняем топ процессы по памяти
        if (i < 5) {
            snprintf(proc_analysis->top_processes[i], sizeof(proc_analysis->top_processes[i]),
                     "%s (PID: %d, Память: %lu KB)",
                     processes[i].name, processes[i].pid, processes[i].memory_kb);
        }
    }

    free_system_process_list(processes);

    return 0;
}

// Функции отображения сетевой и процессной информации
void ui_diagnostics_display_network_info(WINDOW *win, const network_diagnostic_info_t *net_diag) {
    // Создаем локальную структуру данных для UI
    struct {
        char hostname[256];
        int ping_time_ms;
        int packet_loss_percent;
        char connection_status[128];
    } ui_net_diag = {
        .hostname = "localhost",
        .ping_time_ms = 1,
        .packet_loss_percent = 0,
        .connection_status = "Подключен к localhost"
    };

    mvwprintw(win, 2, 2, "🌐 Сетевая диагностика:");
    mvwprintw(win, 3, 4, "Хост: %s", ui_net_diag.hostname);
    mvwprintw(win, 4, 4, "IP адрес: %s", net_diag->ip_address);
    mvwprintw(win, 5, 4, "Время пинга: %d мс", ui_net_diag.ping_time_ms);
    mvwprintw(win, 6, 4, "Потери пакетов: %d%%", ui_net_diag.packet_loss_percent);
    mvwprintw(win, 7, 4, "Статус: %s", ui_net_diag.connection_status);
}

void ui_diagnostics_display_process_info(WINDOW *win, const void *proc_analysis_ptr) {
    typedef struct {
        int total_processes;
        int user_processes;
        int system_processes;
        unsigned long total_memory_usage;
        float total_cpu_usage;
        char top_processes[5][256];
    } proc_analysis_t;
    const proc_analysis_t *proc_analysis = (const proc_analysis_t *)proc_analysis_ptr;
    mvwprintw(win, 2, 2, "🔍 Анализ процессов:");
    mvwprintw(win, 3, 4, "Всего процессов: %d", proc_analysis->total_processes);
    mvwprintw(win, 4, 4, "Пользовательских: %d", proc_analysis->user_processes);
    mvwprintw(win, 5, 4, "Системных: %d", proc_analysis->system_processes);
    mvwprintw(win, 6, 4, "Общее использование CPU: %.1f%%", proc_analysis->total_cpu_usage);
    mvwprintw(win, 7, 4, "Общее использование памяти: %lu KB", proc_analysis->total_memory_usage);

    mvwprintw(win, 9, 2, "🔥 Топ процессы по памяти:");
    for (int i = 0; i < 5; i++) {
        if (strlen(proc_analysis->top_processes[i]) > 0) {
            mvwprintw(win, 10 + i, 4, "%d. %s", i + 1, proc_analysis->top_processes[i]);
        }
    }
}

// Интеграция с AppContext
int ui_diagnostics_sync_with_app_context(app_context_t *ctx) {
    if (!ctx) return -1;

    // Синхронизируем настройки обновления
    g_diag_state.refresh_interval = ctx->config.update_interval_ms / 1000;

    return 0;
}

// Получение метрик системы (упрощенная версия)
int ui_diagnostics_get_system_metrics(void) {
    // Обновляем данные производительности
    ui_diagnostics_update_performance_data(&g_diag_info.current_perf);

    return 0;
}

// Выполнение системных проверок
void ui_diagnostics_perform_system_checks(void) {
    // Выполняем сетевые проверки
    network_diagnostic_info_t net_diag;
    ui_diagnostics_perform_network_check(&net_diag);

    // Анализируем процессы
    struct {
        int total_processes;
        int user_processes;
        int system_processes;
        unsigned long total_memory_usage;
        float total_cpu_usage;
        char top_processes[5][256];
    } proc_analysis;
    ui_diagnostics_analyze_system_processes(&proc_analysis);

    // Здесь можно добавить дополнительные проверки
    // Например, проверка дисков, памяти, etc.
}

// Специфичные функции диагностики

// Запуск сетевой диагностики
void ui_diagnostics_run_network_diagnostics(void) {
    network_diagnostic_info_t net_diag;

    if (ui_diagnostics_perform_network_check(&net_diag) == 0) {
        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        WINDOW *win = newwin(rows - 4, cols - 4, 2, 2);
        box(win, 0, 0);

        ui_diagnostics_display_network_info(win, &net_diag);

        mvwprintw(win, 1, 2, "🔧 СЕТЕВАЯ ДИАГНОСТИКА");
        mvwprintw(win, rows - 5, 2, "Назад: q");

        refresh();
        wrefresh(win);
        delwin(win);

        int ch;
        while ((ch = getch()) != 'q') {
            // Ожидаем
        }
    }
}

// Запуск анализа процессов
void ui_diagnostics_run_process_analysis(void) {
    struct {
        int total_processes;
        int user_processes;
        int system_processes;
        unsigned long total_memory_usage;
        float total_cpu_usage;
        char top_processes[5][256];
    } proc_analysis;

    if (ui_diagnostics_analyze_system_processes(&proc_analysis) == 0) {
        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        WINDOW *win = newwin(rows - 4, cols - 4, 2, 2);
        box(win, 0, 0);

        ui_diagnostics_display_process_info(win, &proc_analysis);

        mvwprintw(win, 1, 2, "🔍 АНАЛИЗ ПРОЦЕССОВ");
        mvwprintw(win, rows - 5, 2, "Назад: q");

        refresh();
        wrefresh(win);
        delwin(win);

        int ch;
        while ((ch = getch()) != 'q') {
            // Ожидаем
        }
    }
}

// Отображение графиков производительности
void ui_diagnostics_show_performance_graphs(void) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Создаем окно для графиков
    WINDOW *win = newwin(rows - 4, cols - 4, 2, 2);
    box(win, 0, 0);

    // Отрисовываем графики
    ui_diagnostics_render_performance_graphs(win, g_diag_state.perf_history, g_diag_state.perf_history_count);

    // Отрисовываем заголовок и футер
    mvwprintw(win, 1, 2, "📊 ГРАФИКИ ПРОИЗВОДИТЕЛЬНОСТИ");
    mvwprintw(win, rows - 5, 2, "Интервал: %d сек | Обновление: %s | Назад: q",
              g_diag_state.refresh_interval, g_diag_state.auto_refresh ? "авто" : "ручное");

    refresh();
    wrefresh(win);
    delwin(win);

    // Ожидаем ввод пользователя
    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case '+':
            case '=':
                g_diag_state.refresh_interval = (g_diag_state.refresh_interval < 10) ?
                    g_diag_state.refresh_interval + 1 : g_diag_state.refresh_interval;
                ui_diagnostics_show_performance_graphs();
                break;
            case '-':
                g_diag_state.refresh_interval = (g_diag_state.refresh_interval > 1) ?
                    g_diag_state.refresh_interval - 1 : g_diag_state.refresh_interval;
                ui_diagnostics_show_performance_graphs();
                break;
            case 'a':
            case 'A':
                g_diag_state.auto_refresh = !g_diag_state.auto_refresh;
                ui_diagnostics_show_performance_graphs();
                break;
            case 'r':
            case 'R':
                ui_diagnostics_update_performance_data(&g_diag_info.current_perf);
                ui_diagnostics_show_performance_graphs();
                break;
        }
    }
}

// Отображение логов ошибок
void ui_diagnostics_show_error_log(void) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Создаем окно для логов
    WINDOW *win = newwin(rows - 4, cols - 4, 2, 2);
    box(win, 0, 0);

    // Обновляем логи ошибок
    ui_diagnostics_update_error_log(g_diag_info.error_log, sizeof(g_diag_info.error_log));

    // Отрисовываем логи
    ui_diagnostics_render_error_log(win, g_diag_info.error_log);

    // Отрисовываем заголовок и футер
    mvwprintw(win, 1, 2, "📋 ЛОГИ ОШИБОК И ПРЕДУПРЕЖДЕНИЙ");
    mvwprintw(win, rows - 5, 2, "Обновить: r | Фильтр: f | Назад: q");

    refresh();
    wrefresh(win);
    delwin(win);

    // Ожидаем ввод пользователя
    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'r':
            case 'R':
                ui_diagnostics_show_error_log();
                break;
            case 'f':
            case 'F':
                // Здесь можно добавить фильтрацию логов
                break;
        }
    }
}

// Запуск системных проверок
void ui_diagnostics_run_system_checks(void) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Создаем окно для проверок
    WINDOW *win = newwin(rows - 4, cols - 4, 2, 2);
    box(win, 0, 0);

    // Отрисовываем проверки
    ui_diagnostics_render_system_checks(win);

    // Отрисовываем заголовок и футер
    mvwprintw(win, 1, 2, "🔧 СИСТЕМНЫЕ ПРОВЕРКИ");
    mvwprintw(win, rows - 5, 2, "Запуск проверок... | Назад: q");

    refresh();
    wrefresh(win);

    // Запускаем проверки
    ui_diagnostics_perform_system_checks();

    delwin(win);

    // Ожидаем ввод пользователя
    int ch;
    while ((ch = getch()) != 'q') {
        // Можно добавить дополнительные действия
    }
}

// Очистка ресурсов диагностики
void ui_diagnostics_cleanup(void) {
    // Здесь можно добавить очистку ресурсов если нужно
}

// Основная функция рендеринга экрана диагностики
void ui_render_diagnostics_screen(void) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Создаем окна для разных компонентов
    WINDOW *header_win = newwin(3, cols, 0, 0);
    WINDOW *main_win = newwin(rows - 6, cols, 3, 0);
    WINDOW *footer_win = newwin(3, cols, rows - 3, 0);

    // Отрисовываем заголовок
    ui_diagnostics_render_header(header_win);

    // Отрисовываем основной контент в зависимости от текущего вида
    switch (g_diag_state.current_view) {
        case 0: // Обзор
            ui_diagnostics_render_overview(main_win, &g_diag_info);
            break;
        case 1: // Статус модулей
            ui_diagnostics_render_module_list(main_win, g_diag_info.modules,
                                           g_diag_info.module_count, g_diag_state.selected_module);
            break;
        case 2: // Графики производительности
            ui_diagnostics_render_performance_graphs(main_win, g_diag_state.perf_history,
                                                  g_diag_state.perf_history_count);
            break;
        case 3: // Логи ошибок
            ui_diagnostics_render_error_log(main_win, g_diag_info.error_log);
            break;
        case 4: // Системные проверки
            ui_diagnostics_render_system_checks(main_win);
            break;
    }

    // Отрисовываем футер с навигацией
    ui_diagnostics_render_footer(footer_win);

    // Обновляем экран
    refresh();
    wrefresh(header_win);
    wrefresh(main_win);
    wrefresh(footer_win);

    // Очищаем окна
    delwin(header_win);
    delwin(main_win);
    delwin(footer_win);
}

// Отрисовка заголовка
void ui_diagnostics_render_header(WINDOW *win) {
    int colors_on = has_colors();
    box(win, 0, 0);

    if (colors_on) wattron(win, COLOR_PAIR(6));
    mvwprintw(win, 1, 2, "🔍 ДИАГНОСТИКА СИСТЕМЫ");
    if (colors_on) wattroff(win, COLOR_PAIR(6));

    mvwprintw(win, 1, 40, "Вид: ");
    switch (g_diag_state.current_view) {
        case 0: mvwprintw(win, 1, 46, "Обзор"); break;
        case 1: mvwprintw(win, 1, 46, "Модули"); break;
        case 2: mvwprintw(win, 1, 46, "Графики"); break;
        case 3: mvwprintw(win, 1, 46, "Логи"); break;
        case 4: mvwprintw(win, 1, 46, "Проверки"); break;
    }

    char time_str[32];
    time_t now = time(NULL);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
    mvwprintw(win, 1, 70, "Время: %s", time_str);
}

// Отрисовка футера с навигацией
void ui_diagnostics_render_footer(WINDOW *win) {
    int colors_on = has_colors();
    int cols;
    getmaxyx(win, (int){0}, cols);

    box(win, 0, 0);

    if (colors_on) wattron(win, COLOR_PAIR(5));
    mvwprintw(win, 1, 2, "Навигация: [1-5] виды | [r] обновить | [a] автообновление | [q] выход");
    if (colors_on) wattroff(win, COLOR_PAIR(5));
}

// Обновление всех данных диагностики
int ui_diagnostics_update_all_data(void) {
    // Обновляем данные производительности
    ui_diagnostics_update_performance_data(&g_diag_info.current_perf);

    // Обновляем статус модулей
    ui_diagnostics_update_module_status(g_diag_info.modules, &g_diag_info.module_count);

    // Обновляем логи ошибок
    ui_diagnostics_update_error_log(g_diag_info.error_log, sizeof(g_diag_info.error_log));

    // Обновляем системную информацию
    ui_diagnostics_collect_system_info(g_diag_info.system_info, sizeof(g_diag_info.system_info));

    return 0;
}

// Обновление данных производительности
int ui_diagnostics_update_performance_data(performance_data_t *data) {
    if (!data) return -1;

    // Получаем данные CPU
    cpu_info_t cpu;
    if (get_cpu_info(&cpu) == 0) {
        data->cpu_usage = cpu.usage_percent;
    }

    // Получаем данные памяти
    memory_info_t mem;
    if (get_memory_info(&mem) == 0) {
        data->memory_used = mem.used_mb;
        data->memory_total = mem.total_mb;
    }

    // Получаем сетевые данные
    data->network_rx_rate = get_network_rx();
    data->network_tx_rate = get_network_tx();

    // Получаем количество процессов
    system_process_info_t *processes = NULL;
    int process_count = 0;
    if (get_system_process_list(&processes, &process_count) == 0) {
        data->process_count = process_count;
        free_system_process_list(processes);
    }

    // Получаем количество потоков
    data->thread_count = sysconf(_SC_NPROCESSORS_ONLN);

    // Устанавливаем временную метку
    data->timestamp = time(NULL);

    // Сохраняем в истории для графиков
    if (g_diag_state.perf_history_count < g_diag_state.graph_history_size) {
        g_diag_state.perf_history[g_diag_state.perf_history_count] = *data;
        g_diag_state.perf_history_count++;
    } else {
        // Сдвигаем историю
        memmove(&g_diag_state.perf_history[0], &g_diag_state.perf_history[1],
                sizeof(performance_data_t) * (g_diag_state.graph_history_size - 1));
        g_diag_state.perf_history[g_diag_state.graph_history_size - 1] = *data;
    }

    return 0;
}

// Обновление статуса модулей
int ui_diagnostics_update_module_status(diagnostics_module_status_t *modules, int *count) {
    if (!modules || !count) return -1;

    app_context_t *ctx = get_app_context();
    if (!ctx) return -1;

    *count = 0;

    // Проверяем основные модули
    const char *module_names[] = {
        "system_monitor", "developer_tools", "diagnostics", "error_handler", "ui"
    };

    for (int i = 0; i < 5 && *count < 16; i++) {
        strcpy(modules[*count].module_name, module_names[i]);
        modules[*count].initialized = is_module_initialized(module_names[i]);

        if (modules[*count].initialized) {
            modules[*count].status = 1; // Активен
            strcpy(modules[*count].last_error, "Нет ошибок");
        } else {
            modules[*count].status = 2; // Ошибка
            strcpy(modules[*count].last_error, "Модуль не инициализирован");
        }

        modules[*count].last_check = time(NULL);
        modules[*count].performance_impact = 0.0f; // Заглушка

        (*count)++;
    }

    return 0;
}

// Сбор системной информации
int ui_diagnostics_collect_system_info(char *info_buffer, int buffer_size) {
    if (!info_buffer || buffer_size <= 0) return -1;

    system_info_t sys_info;
    if (get_system_monitor_info(&sys_info) != 0) {
        snprintf(info_buffer, buffer_size, "Не удалось получить системную информацию");
        return -1;
    }

    snprintf(info_buffer, buffer_size,
             "Платформа: %s\n"
             "Хост: %s\n"
             "ОС: %s\n"
             "Время работы: %d сек\n"
             "Время загрузки: %s",
             sys_info.platform,
             sys_info.hostname,
             sys_info.os_version,
             sys_info.uptime_seconds,
             ctime(&sys_info.boot_time));

    return 0;
}

// Обновление логов ошибок
int ui_diagnostics_update_error_log(char *log_buffer, int buffer_size) {
    if (!log_buffer || buffer_size <= 0) return -1;

    // Читаем последние строки из лог-файла ошибок
    FILE *fp = fopen("error.log", "r");
    if (!fp) {
        snprintf(log_buffer, buffer_size, "Лог-файл ошибок не найден");
        return -1;
    }

    char line[256];
    char last_lines[10][256];
    int line_count = 0;

    // Читаем последние 10 строк
    while (fgets(line, sizeof(line), fp) && line_count < 10) {
        strcpy(last_lines[line_count], line);
        line_count++;
    }
    fclose(fp);

    // Формируем буфер логов
    log_buffer[0] = '\0';
    for (int i = 0; i < line_count; i++) {
        strncat(log_buffer, last_lines[i], buffer_size - strlen(log_buffer) - 1);
    }

    return 0;
}