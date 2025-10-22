/**
 * UI Data Visualizer Module Implementation
 *
 * Реализация бизнес-логики отображения системных данных и метрик.
 */

#include "ui/ui_data_visualizer.h"
#include "ui/ui_renderer.h"
#include "ui/ui_input_handler.h"
#include "core/app_context.h"
#include "modules/system_monitor.h"
#include "modules/developer_tools.h"
#include "modules/diagnostics.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// Структура для хранения данных визуализации через контекст приложения
typedef struct {
    cpu_info_t cpu_info;
    memory_info_t memory_info;
    system_info_t system_info;
    system_process_info_t *process_list;
    int process_count;
    diagnostic_result_t *diag_results;
    int diag_count;
    bool modules_initialized;
} visualizer_context_t;

static visualizer_context_t g_visualizer_ctx = {0};

void ui_data_visualizer_init(void) {
    // Инициализация модуля визуализации данных через контекст приложения
    memset(&g_visualizer_ctx, 0, sizeof(visualizer_context_t));

    // Инициализируем модули через контекст приложения
    app_context_t *ctx = get_app_context();
    if (!ctx) {
        LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось получить контекст приложения", "ui_data_visualizer");
        return;
    }

    // Инициализируем модули если они еще не инициализированы
    if (!is_module_initialized("system_monitor")) {
        if (initialize_module("system_monitor") != 0) {
            LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать модуль system_monitor", "ui_data_visualizer");
        }
    }

    if (!is_module_initialized("developer_tools")) {
        if (initialize_module("developer_tools") != 0) {
            LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать модуль developer_tools", "ui_data_visualizer");
        }
    }

    if (!is_module_initialized("diagnostics")) {
        if (initialize_module("diagnostics") != 0) {
            LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать модуль diagnostics", "ui_data_visualizer");
        }
    }

    g_visualizer_ctx.modules_initialized = true;
    LOG_INFO("Модуль визуализации данных инициализирован", "ui_data_visualizer");
}

void ui_data_visualizer_cleanup(void) {
    // Очистка ресурсов модуля визуализации данных
    if (g_visualizer_ctx.process_list) {
        // TODO: Добавить функцию освобождения списка процессов в заголовочный файл
        // free_process_list(g_visualizer_ctx.process_list);
        free(g_visualizer_ctx.process_list);
        g_visualizer_ctx.process_list = NULL;
    }

    if (g_visualizer_ctx.diag_results) {
        free(g_visualizer_ctx.diag_results);
        g_visualizer_ctx.diag_results = NULL;
    }

    memset(&g_visualizer_ctx, 0, sizeof(visualizer_context_t));
    LOG_INFO("Модуль визуализации данных очищен", "ui_data_visualizer");
}

void ui_show_main_screen(const system_metrics_t *metrics,
                         const metrics_history_t *history, int colors_on) {
    if (!metrics) return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Получаем данные из модулей через контекст приложения
    app_context_t *ctx = get_app_context();
    if (!ctx || !g_visualizer_ctx.modules_initialized) {
        // Простой fallback к старым данным если модули недоступны
        // Используем встроенную реализацию fallback
        {
            if (!metrics) return;

            int rows, cols;
            getmaxyx(stdscr, rows, cols);

            // Отрисовка заголовка (fallback)
            char title[64];
            snprintf(title, sizeof(title), "CPU %.1f%% MEM %.1f%%", metrics->cpu_usage, metrics->memory_usage);
            ui_draw_modern_header(title);

            // Отображение основных метрик с помощью рендерера (fallback)
            ui_draw_labeled_value(3, 2, "🖥️", "Процессор", "%.2f%%", colors_on ? 2 : 0);
            int bar_width = cols > 90 ? 40 : 20;
            ui_draw_gradient_bar(4, 2, bar_width, metrics->cpu_usage, 0);

            ui_draw_labeled_value(13, 2, "💾", "Память", "%.2f%%", colors_on ? 2 : 0);
            ui_draw_gradient_bar(14, 2, bar_width, metrics->memory_usage, 0);

            // Отображение истории CPU и памяти (fallback)
            if (history) {
                ui_draw_history_frame(6, 50, "ИСТОРИЯ ЗАГРУЗКИ");
                ui_draw_sparkline(7, 52, history->cpu_history, 60, 50, colors_on ? 2 : 0);
                ui_draw_sparkline(9, 52, history->memory_history, 60, 50, colors_on ? 2 : 0);
            }

            // Отображение ядер процессора (fallback)
            if (metrics->core_count > 0) {
                ui_draw_modern_box(20, 2, metrics->core_count + 2, 70);

                for (int i = 0; i < metrics->core_count && i < 8; i++) {
                    char core_label[16];
                    snprintf(core_label, sizeof(core_label), "Ядро %d", i);

                    char core_value[32];
                    snprintf(core_value, sizeof(core_value), "%.1f%%",
                            metrics->core_temperatures[i]);

                    ui_draw_labeled_value(21 + i, 4, NULL, core_label, core_value, colors_on ? 3 : 0);
                }
            }

            // Статусная строка (fallback)
            char status_text[128];
            snprintf(status_text, sizeof(status_text), "Время работы: %ld сек | Ядер: %d",
                      metrics->uptime_seconds, metrics->core_count);

            ui_draw_status_bar(rows - 3, 2, status_text);
        }
        return;
    }

    // Получаем актуальные данные из модулей
    if (get_cpu_info(&g_visualizer_ctx.cpu_info) == 0 &&
        get_memory_info(&g_visualizer_ctx.memory_info) == 0 &&
        get_system_monitor_info(&g_visualizer_ctx.system_info) == 0) {

        // Отрисовка заголовка с данными из модулей
        char title[64];
        snprintf(title, sizeof(title), "CPU %.1f%% MEM %.1f%%",
                g_visualizer_ctx.cpu_info.usage_percent,
                g_visualizer_ctx.memory_info.usage_percent);
        ui_draw_modern_header(title);

        // Отображение основных метрик с помощью рендерера
        ui_draw_labeled_value(3, 2, "🖥️", "Процессор", "%.2f%%", colors_on ? 2 : 0);
        int bar_width = cols > 90 ? 40 : 20;
        ui_draw_gradient_bar(4, 2, bar_width, g_visualizer_ctx.cpu_info.usage_percent, 0);

        ui_draw_labeled_value(13, 2, "💾", "Память", "%.2f%%", colors_on ? 2 : 0);
        ui_draw_gradient_bar(14, 2, bar_width, g_visualizer_ctx.memory_info.usage_percent, 0);

        // Отображение информации о системе
        char status_text[128];
        snprintf(status_text, sizeof(status_text), "Время работы: %d сек | Ядер: %d | Платформа: %s",
                g_visualizer_ctx.system_info.uptime_seconds,
                g_visualizer_ctx.cpu_info.core_count,
                g_visualizer_ctx.system_info.platform);

        ui_draw_status_bar(rows - 3, 2, status_text);

        // Отображение ядер процессора с температурами
        if (g_visualizer_ctx.cpu_info.core_count > 0) {
            ui_draw_modern_box(20, 2, g_visualizer_ctx.cpu_info.core_count + 2, 70);

            for (int i = 0; i < g_visualizer_ctx.cpu_info.core_count && i < 8; i++) {
                char core_label[16];
                snprintf(core_label, sizeof(core_label), "Ядро %d", i);

                char core_value[32];
                snprintf(core_value, sizeof(core_value), "%.1f%% / %.1f°C",
                        100.0f / g_visualizer_ctx.cpu_info.core_count, // Заглушка для загрузки ядра
                        g_visualizer_ctx.cpu_info.temperature);

                ui_draw_labeled_value(21 + i, 4, NULL, core_label, core_value, colors_on ? 3 : 0);
            }
        }

        // Отображение истории CPU и памяти (если доступна)
        if (history) {
            ui_draw_history_frame(6, 50, "ИСТОРИЯ ЗАГРУЗКИ");
            ui_draw_sparkline(7, 52, history->cpu_history, 60, 50, colors_on ? 2 : 0);
            ui_draw_sparkline(9, 52, history->memory_history, 60, 50, colors_on ? 2 : 0);
        }
    } else {
        // Простой fallback если не удалось получить данные из модулей
        // Используем встроенную реализацию fallback
        {
            if (!metrics) return;

            int rows, cols;
            getmaxyx(stdscr, rows, cols);

            // Отрисовка заголовка (fallback)
            char title[64];
            snprintf(title, sizeof(title), "CPU %.1f%% MEM %.1f%%", metrics->cpu_usage, metrics->memory_usage);
            ui_draw_modern_header(title);

            // Отображение основных метрик с помощью рендерера (fallback)
            ui_draw_labeled_value(3, 2, "🖥️", "Процессор", "%.2f%%", colors_on ? 2 : 0);
            int bar_width = cols > 90 ? 40 : 20;
            ui_draw_gradient_bar(4, 2, bar_width, metrics->cpu_usage, 0);

            ui_draw_labeled_value(13, 2, "💾", "Память", "%.2f%%", colors_on ? 2 : 0);
            ui_draw_gradient_bar(14, 2, bar_width, metrics->memory_usage, 0);

            // Отображение истории CPU и памяти (fallback)
            if (history) {
                ui_draw_history_frame(6, 50, "ИСТОРИЯ ЗАГРУЗКИ");
                ui_draw_sparkline(7, 52, history->cpu_history, 60, 50, colors_on ? 2 : 0);
                ui_draw_sparkline(9, 52, history->memory_history, 60, 50, colors_on ? 2 : 0);
            }

            // Отображение ядер процессора (fallback)
            if (metrics->core_count > 0) {
                ui_draw_modern_box(20, 2, metrics->core_count + 2, 70);

                for (int i = 0; i < metrics->core_count && i < 8; i++) {
                    char core_label[16];
                    snprintf(core_label, sizeof(core_label), "Ядро %d", i);

                    char core_value[32];
                    snprintf(core_value, sizeof(core_value), "%.1f%%",
                            metrics->core_temperatures[i]);

                    ui_draw_labeled_value(21 + i, 4, NULL, core_label, core_value, colors_on ? 3 : 0);
                }
            }

            // Статусная строка (fallback)
            char status_text[128];
            snprintf(status_text, sizeof(status_text), "Время работы: %ld сек | Ядер: %d",
                      metrics->uptime_seconds, metrics->core_count);

            ui_draw_status_bar(rows - 3, 2, status_text);
        }
    }
}

void ui_show_processes_screen(input_state_t *state, int colors_on) {
    if (!state) return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Отрисовка рамки для списка процессов
    ui_draw_modern_box(1, 1, rows - 4, cols - 2);

    // Заголовок
    ui_draw_modern_header("СПИСОК ПРОЦЕССОВ");

    // Здесь будет логика получения и отображения списка процессов
    // Пока заглушка
    mvprintw(3, 3, "Процессы: %d элементов", state->total_items);
    mvprintw(4, 3, "Выбрано: %d", state->selected_item);
    mvprintw(5, 3, "Фильтр: '%s'", state->filter_text);
}

void ui_show_process_tree_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("ДЕРЕВО ПРОЦЕССОВ");

    // Здесь будет логика отображения дерева процессов
    // Пока заглушка
    mvprintw(3, 3, "Отображение дерева процессов...");
}

void ui_show_network_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("СЕТЕВАЯ ИНФОРМАЦИЯ");

    char net_info[256] = "";
    ui_get_network_info(net_info, sizeof(net_info));

    mvprintw(3, 3, "Сеть: %s", net_info);
}

void ui_show_diagnostics_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("ДИАГНОСТИКА СИСТЕМЫ");

    // Здесь будет логика отображения диагностической информации
    // Пока заглушка
    mvprintw(3, 3, "Диагностика системы...");
}

void ui_show_testing_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("ТЕСТИРОВАНИЕ СИСТЕМЫ");

    // Проверка модулей через AppContext
    app_context_t *ctx = get_app_context();
    if (ctx) {
        int y = 5;

        // Проверка system_monitor модуля
        mvprintw(y, 3, "System Monitor: ");
        if (is_module_initialized("system_monitor")) {
            attron(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 18, "[✓]");
            attroff(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 22, "Инициализирован");
        } else {
            attron(COLOR_PAIR(COLOR_PAIR_RED));
            mvprintw(y, 18, "[✗]");
            attroff(COLOR_PAIR(COLOR_PAIR_RED));
            mvprintw(y, 22, "Не найден");
        }
        y += 2;

        // Проверка developer_tools модуля
        mvprintw(y, 3, "Developer Tools: ");
        if (is_module_initialized("developer_tools")) {
            attron(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 19, "[✓]");
            attroff(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 23, "Инициализирован");
        } else {
            attron(COLOR_PAIR(COLOR_PAIR_YELLOW));
            mvprintw(y, 19, "[!]");
            attroff(COLOR_PAIR(COLOR_PAIR_YELLOW));
            mvprintw(y, 23, "Не обязателен");
        }
        y += 2;

        // Проверка diagnostics модуля
        mvprintw(y, 3, "Diagnostics: ");
        if (is_module_initialized("diagnostics")) {
            attron(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 15, "[✓]");
            attroff(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 19, "Инициализирован");
        } else {
            attron(COLOR_PAIR(COLOR_PAIR_YELLOW));
            mvprintw(y, 15, "[!]");
            attroff(COLOR_PAIR(COLOR_PAIR_YELLOW));
            mvprintw(y, 19, "Не обязателен");
        }
        y += 2;

        // Проверка error_handler модуля
        mvprintw(y, 3, "Error Handler: ");
        if (is_module_initialized("error_handler")) {
            attron(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 16, "[✓]");
            attroff(COLOR_PAIR(COLOR_PAIR_GREEN));
            mvprintw(y, 20, "Инициализирован");
        } else {
            attron(COLOR_PAIR(COLOR_PAIR_RED));
            mvprintw(y, 16, "[✗]");
            attroff(COLOR_PAIR(COLOR_PAIR_RED));
            mvprintw(y, 20, "Ошибка");
        }
        y += 2;

        // Отображение общей информации о контексте
        mvprintw(y, 3, "Статус приложения: %s", get_app_status_string(ctx->global_state.status));
        y += 2;
        mvprintw(y, 3, "Режим работы: %s", get_app_mode_string(ctx->global_state.mode));
        y += 2;
        mvprintw(y, 3, "Активных модулей: %d", ctx->modules.active_modules_count);

        // Показываем кнопку продолжить
        y = rows - 6;
        mvprintw(y, 3, "Нажмите [ENTER] или [SPACE] для продолжения...");
    } else {
        mvprintw(5, 3, "Ошибка: не удалось получить контекст приложения");
        mvprintw(rows - 6, 3, "Нажмите [ENTER] или [SPACE] для продолжения...");
    }
}

void ui_show_weather_details_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("ДЕТАЛЬНАЯ ПОГОДА");

    char weather_info[256] = "";
    ui_get_weather_info(weather_info, sizeof(weather_info), NULL);

    mvprintw(3, 3, "Погода: %s", weather_info);
}

void ui_show_docker_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("DOCKER КОНТЕЙНЕРЫ");

    // Здесь будет логика отображения Docker контейнеров
    // Пока заглушка
    mvprintw(3, 3, "Docker контейнеры...");
}

void ui_show_git_repos_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("GIT РЕПОЗИТОРИИ");

    // Здесь будет логика отображения Git репозиториев
    // Пока заглушка
    mvprintw(3, 3, "Git репозитории...");
}

void ui_show_dev_environment_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("СРЕДА РАЗРАБОТКИ");

    // Здесь будет логика отображения информации о среде разработки
    // Пока заглушка
    mvprintw(3, 3, "Среда разработки...");
}

void ui_show_listening_ports_screen(int colors_on) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    ui_draw_modern_box(1, 1, rows - 4, cols - 2);
    ui_draw_modern_header("ЗАНЯТЫЕ ПОРТЫ");

    // Здесь будет логика отображения занятых портов
    // Пока заглушка
    mvprintw(3, 3, "Занятые порты...");
}

void ui_update_metrics_history(metrics_history_t *history, float cpu, float memory) {
    if (!history) return;

    history->cpu_history[history->history_index] = cpu;
    history->memory_history[history->history_index] = memory;
    history->history_index = (history->history_index + 1) % 60;
}

// Функция объявлена выше в файле

// Fallback функция для отображения главного экрана при недоступности модулей
void ui_show_main_screen_fallback(const system_metrics_t *metrics,
                                  const metrics_history_t *history, int colors_on) {
    if (!metrics) return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Отрисовка заголовка (fallback)
    char title[64];
    snprintf(title, sizeof(title), "CPU %.1f%% MEM %.1f%%", metrics->cpu_usage, metrics->memory_usage);
    ui_draw_modern_header(title);

    // Отображение основных метрик с помощью рендерера (fallback)
    ui_draw_labeled_value(3, 2, "🖥️", "Процессор", "%.2f%%", colors_on ? 2 : 0);
    int bar_width = cols > 90 ? 40 : 20;
    ui_draw_gradient_bar(4, 2, bar_width, metrics->cpu_usage, 0);

    ui_draw_labeled_value(13, 2, "💾", "Память", "%.2f%%", colors_on ? 2 : 0);
    ui_draw_gradient_bar(14, 2, bar_width, metrics->memory_usage, 0);

    // Отображение истории CPU и памяти (fallback)
    if (history) {
        ui_draw_history_frame(6, 50, "ИСТОРИЯ ЗАГРУЗКИ");
        ui_draw_sparkline(7, 52, history->cpu_history, 60, 50, colors_on ? 2 : 0);
        ui_draw_sparkline(9, 52, history->memory_history, 60, 50, colors_on ? 2 : 0);
    }

    // Отображение ядер процессора (fallback)
    if (metrics->core_count > 0) {
        ui_draw_modern_box(20, 2, metrics->core_count + 2, 70);

        for (int i = 0; i < metrics->core_count && i < 8; i++) {
            char core_label[16];
            snprintf(core_label, sizeof(core_label), "Ядро %d", i);

            char core_value[32];
            snprintf(core_value, sizeof(core_value), "%.1f%%",
                    metrics->core_temperatures[i]);

            ui_draw_labeled_value(21 + i, 4, NULL, core_label, core_value, colors_on ? 3 : 0);
        }
    }

    // Статусная строка (fallback)
    char status_text[128];
    snprintf(status_text, sizeof(status_text), "Время работы: %ld сек | Ядер: %d",
              metrics->uptime_seconds, metrics->core_count);

    ui_draw_status_bar(rows - 3, 2, status_text);
}

int ui_get_system_metrics(system_metrics_t *metrics) {
    if (!metrics) return -1;

    app_context_t *ctx = get_app_context();
    if (!ctx || !g_visualizer_ctx.modules_initialized) {
        // Простой fallback к старым функциям если модули недоступны
        {
            if (!metrics) return -1;

            // Внешние функции из других модулей (старые)
            extern float get_cpu_usage(void);
            extern float get_memory_usage(void);
            extern long get_uptime_seconds(void);
            extern int get_cpu_frequencies(long long *cur, long long *max);
            extern int smc_get_core_temperatures(float *temps, int max_temps, int *wrote);
            extern float smc_get_mem_temperature(float *temp);

            metrics->cpu_usage = get_cpu_usage();
            metrics->memory_usage = get_memory_usage();
            metrics->uptime_seconds = get_uptime_seconds();

            // Получение частот CPU
            long long cur_freq, max_freq;
            if (get_cpu_frequencies(&cur_freq, &max_freq) == 0) {
                snprintf(metrics->cpu_freq, sizeof(metrics->cpu_freq), "%.2f GHz",
                        (double)cur_freq / 1e9);
            }

            // Получение температур ядер
            metrics->temp_count = 0;
            if (smc_get_core_temperatures(metrics->core_temperatures, 16, &metrics->temp_count) != 0) {
                metrics->temp_count = 0;
            }

            // Получение температуры памяти
            smc_get_mem_temperature(&metrics->memory_temp);

            // Получение количества ядер (пока заглушка)
            metrics->core_count = 4;

            return 0;
        }
    }

    // Используем модули для получения метрик
    if (get_cpu_info(&g_visualizer_ctx.cpu_info) == 0) {
        metrics->cpu_usage = g_visualizer_ctx.cpu_info.usage_percent;
        metrics->core_count = g_visualizer_ctx.cpu_info.core_count;
        // Здесь можно добавить конвертацию других полей CPU
    }

    if (get_memory_info(&g_visualizer_ctx.memory_info) == 0) {
        metrics->memory_usage = g_visualizer_ctx.memory_info.usage_percent;
        // Здесь можно добавить конвертацию других полей памяти
    }

    if (get_system_monitor_info(&g_visualizer_ctx.system_info) == 0) {
        metrics->uptime_seconds = g_visualizer_ctx.system_info.uptime_seconds;
        // Здесь можно добавить конвертацию других полей системы
    }

    // Получение списка процессов для дополнительной информации
    if (g_visualizer_ctx.process_list) {
        free_system_process_list(g_visualizer_ctx.process_list);
    }
    get_system_process_list(&g_visualizer_ctx.process_list, &g_visualizer_ctx.process_count);

    return 0;
}

// Fallback функция для получения системных метрик
int ui_get_system_metrics_fallback(system_metrics_t *metrics) {
    if (!metrics) return -1;

    // Внешние функции из других модулей (старые)
    extern float get_cpu_usage(void);
    extern float get_memory_usage(void);
    extern long get_uptime_seconds(void);
    extern int get_cpu_frequencies(long long *cur, long long *max);
    extern int smc_get_core_temperatures(float *temps, int max_temps, int *wrote);
    extern float smc_get_mem_temperature(float *temp);

    metrics->cpu_usage = get_cpu_usage();
    metrics->memory_usage = get_memory_usage();
    metrics->uptime_seconds = get_uptime_seconds();

    // Получение частот CPU
    long long cur_freq, max_freq;
    if (get_cpu_frequencies(&cur_freq, &max_freq) == 0) {
        snprintf(metrics->cpu_freq, sizeof(metrics->cpu_freq), "%.2f GHz",
                (double)cur_freq / 1e9);
    }

    // Получение температур ядер
    metrics->temp_count = 0;
    if (smc_get_core_temperatures(metrics->core_temperatures, 16, &metrics->temp_count) != 0) {
        metrics->temp_count = 0;
    }

    // Получение температуры памяти
    smc_get_mem_temperature(&metrics->memory_temp);

    // Получение количества ядер (пока заглушка)
    metrics->core_count = 4;

    return 0;
}

int ui_detect_anomalies(const system_metrics_t *current,
                       const metrics_history_t *history, float sigma_threshold) {
    if (!current || !history) return 0;

    // Простая проверка на аномалии
    int anomalies = 0;

    // Проверка CPU
    if (current->cpu_usage > 90.0f) {
        anomalies |= 1;
    }

    // Проверка памяти
    if (current->memory_usage > 95.0f) {
        anomalies |= 2;
    }

    return anomalies;
}

int ui_check_thresholds(const system_metrics_t *metrics,
                       float cpu_threshold, float memory_threshold, float temp_threshold) {
    if (!metrics) return 0;

    int critical = 0;

    if (metrics->cpu_usage >= cpu_threshold) {
        critical |= 1; // Критическая загрузка CPU
    }

    if (metrics->memory_usage >= memory_threshold) {
        critical |= 2; // Критическая загрузка памяти
    }

    // Проверка температур
    for (int i = 0; i < metrics->temp_count; i++) {
        if (metrics->core_temperatures[i] >= temp_threshold) {
            critical |= 4; // Критическая температура
            break;
        }
    }

    return critical;
}

int ui_save_system_snapshot(const char *filename, const system_metrics_t *metrics) {
    if (!filename || !metrics) return -1;

    FILE *fp = fopen(filename, "w");
    if (!fp) return -1;

    time_t now = time(NULL);
    fprintf(fp, "Снимок системы от %s", ctime(&now));
    fprintf(fp, "CPU: %.2f%%\n", metrics->cpu_usage);
    fprintf(fp, "Память: %.2f%%\n", metrics->memory_usage);
    fprintf(fp, "Время работы: %ld сек\n", metrics->uptime_seconds);

    fclose(fp);
    return 0;
}


void ui_get_weather_info(char *buffer, size_t buffer_size, const char *city) {
    if (!buffer) return;

    // Заглушка для получения информации о погоде
    snprintf(buffer, buffer_size, "Погода недоступна");
}

void ui_get_network_info(char *buffer, size_t buffer_size) {
    if (!buffer) return;

    char net_info[256];
    ui_get_network_info(net_info, sizeof(net_info));
    snprintf(buffer, buffer_size, "%s", net_info);
}

void ui_get_disk_info(char *buffer, size_t buffer_size) {
    if (!buffer) return;

    char disk_info[256];
    ui_get_disk_info(disk_info, sizeof(disk_info));
    snprintf(buffer, buffer_size, "%s", disk_info);
}