/**
 * UI Renderer Module Implementation
 *
 * Реализация функций отрисовки графики и визуальных элементов.
 */

#include "ui_renderer.h"
#include "ui_theme.h"  // Интеграция системы тем
#include <ncurses.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void ui_renderer_init(void) {
    // Инициализация выполняется в main UI модуле
    // Здесь только специфичные настройки рендеринга
}

void ui_renderer_cleanup(void) {
    // Очистка ресурсов рендеринга при необходимости
}

void ui_printw_clip(int y, int x, const char *fmt, ...) {
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
        buf[maxlen] = '\0';
    }
    mvaddstr(y, x, buf);
}

void ui_printw_clip_color(int y, int x, int color_pair, const char *fmt, ...) {
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
        buf[maxlen] = '\0';
    }

    attron(COLOR_PAIR(color_pair));
    mvaddstr(y, x, buf);
    attroff(COLOR_PAIR(color_pair));
}

void ui_draw_hsep(int y, int x) {
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    if (y < 0 || y >= rows) return;

    int len = cols - x - 1;
    if (len <= 0) return;

    for (int i = 0; i < len; i++) {
        mvaddch(y, x + i, '-');
    }
}

void ui_render_bar(int y, int x, int width, float percent, int colors_on) {
    if (width <= 0) return;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    int filled = (int)((percent / 100.0f) * width);

    // Используем цвета из системы тем
    int color_pair;
    if (percent >= 90.0f) color_pair = COLOR_BTOP_GRAD5;     // красный (критично)
    else if (percent >= 75.0f) color_pair = COLOR_BTOP_GRAD4; // желтый (высокий)
    else if (percent >= 50.0f) color_pair = COLOR_BTOP_GRAD3; // зеленый (средний)
    else if (percent >= 25.0f) color_pair = COLOR_BTOP_GRAD2; // голубой (низкий)
    else color_pair = COLOR_BTOP_GRAD1;                      // синий (очень низкий)

    if (colors_on) {
        attron(COLOR_PAIR(current_theme.color_pairs[color_pair]));
    }

    // Используем символы из темы
    char fill_char = symbols.bar_full[0];
    char empty_char = symbols.bar_empty[0];

    for (int i = 0; i < width; i++) {
        if (i < filled) {
            // Градиент: разные символы для заполнения
            if (percent >= 95.0f) fill_char = symbols.bar_full[0];
            else if (percent >= 85.0f) fill_char = symbols.bar_partial3[0];
            else if (percent >= 70.0f) fill_char = symbols.bar_partial2[0];
            else if (percent >= 50.0f) fill_char = symbols.bar_partial1[0];
            else fill_char = symbols.bar_full[0];
        } else {
            fill_char = empty_char;
        }
        mvaddch(y, x + i, fill_char);
    }

    if (colors_on) {
        attroff(COLOR_PAIR(current_theme.color_pairs[color_pair]));
    }
}

int ui_choose_color_by_percent(float percent, float warn_threshold, float crit_threshold) {
    if (percent >= crit_threshold) return COLOR_PAIR_RED;   // красный
    if (percent >= warn_threshold) return COLOR_PAIR_YELLOW; // желтый
    return COLOR_PAIR_GREEN;                                 // зеленый
}



void ui_draw_sparkline(int y, int x, const float *values, int count, int width, int color_pair) {
    if (!values || count <= 0 || width <= 0) return;

    // Используем символы из системы тем для графиков
    const char *levels;
    if (current_theme.unicode_icons) {
        levels = " ▁▂▃▄▅▆▇█";
    } else {
        levels = " .:-=+*#%@";
    }

    // Используем цвет из системы тем или переданный цвет
    int theme_color = color_pair > 0 ? color_pair : COLOR_BTOP_ACCENT1;

    for (int i = 0; i < width && i < count; i++) {
        int value_idx = i * count / width;
        float value = values[value_idx];

        int lvl = (int)(value / 11.11f); // 100/9 уровней
        if (lvl < 0) lvl = 0;
        if (lvl > 8) lvl = 8;

        if (has_colors()) {
            attron(COLOR_PAIR(current_theme.color_pairs[theme_color]));
        }
        mvaddch(y, x + i, levels[lvl]);
        if (has_colors()) {
            attroff(COLOR_PAIR(current_theme.color_pairs[theme_color]));
        }
    }
}

void ui_draw_labeled_value(int y, int x, const char *icon, const char *label,
                          const char *value, int color_pair) {
    char buf[256] = "";

    if (icon) {
        strcat(buf, icon);
        strcat(buf, " ");
    }

    if (label) {
        strcat(buf, label);
        strcat(buf, ": ");
    }

    if (value) {
        strcat(buf, value);
    }

    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
        mvprintw(y, x, "%s", buf);
        attroff(COLOR_PAIR(color_pair));
    } else {
        mvprintw(y, x, "%s", buf);
    }
}

void ui_draw_status_bar(int y, int x, const char *module_status) {
    if (!module_status) return;

    mvprintw(y, x, "%s", module_status);
}

void ui_draw_history_frame(int y, int x, const char *title) {
    if (!title) return;

    // Используем цвета из системы тем для рамки
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Верхняя граница
    mvprintw(y-1, x-1, "%s %s ", symbols.corner_tl, title);
    int title_len = strlen(title);
    for (int i = 0; i < 50 - title_len; i++) {
        mvaddch(y-1, x + 3 + title_len + i, symbols.border_h[0]);
    }
    mvaddch(y-1, x + 52, symbols.corner_tr[0]);

    // Боковые границы
    for (int i = 0; i < 5; i++) {
        mvaddch(y + i, x - 1, symbols.border_v[0]);
        mvaddch(y + i, x + 53, symbols.border_v[0]);
    }

    // Нижняя граница
    mvaddch(y + 4, x - 1, symbols.corner_bl[0]);
    for (int i = 0; i < 53; i++) {
        mvaddch(y + 4, x + i, symbols.border_h[0]);
    }
    mvaddch(y + 4, x + 53, symbols.corner_br[0]);

    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Метки
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
    }
    mvprintw(y, x + 1, "ЦП:");
    mvprintw(y + 2, x + 1, "ПАМЯТЬ:");
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
    }
}

void ui_render_testing_screen(int rows, int cols, float progress,
                             const char *module_status, int colors_on) {
    if (rows <= 0 || cols <= 0) return;

    // Очищаем экран
    clear();

    // Заголовок экрана тестирования
    char title[64];
    snprintf(title, sizeof(title), "ИНИЦИАЛИЗАЦИЯ СИСТЕМЫ МОНИТОРИНГА");
    ui_draw_modern_header(title);

    // Прогресс-бар инициализации
    int bar_width = cols > 80 ? 50 : 30;
    int bar_x = (cols - bar_width) / 2;
    int bar_y = 5;

    // Используем цвета из системы тем
    if (colors_on) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
    }
    mvprintw(bar_y - 1, bar_x - 2, "Прогресс: %.1f%%", progress);
    if (colors_on) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
    }

    // Используем функцию рендеринга бара с темами
    ui_render_bar(bar_y, bar_x, bar_width, progress, colors_on);

    // Отображение статуса модулей (если предоставлен)
    if (module_status) {
        if (colors_on) {
            attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
        }
        mvprintw(bar_y + 3, 2, "Статус модулей мониторинга:");
        if (colors_on) {
            attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
        }

        if (colors_on) {
            attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
        }
        mvprintw(bar_y + 4, 4, "%s", module_status);
        if (colors_on) {
            attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
        }
    }

    // Инструкции для пользователя
    int instr_y = rows - 8;
    if (colors_on) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_INFO]));
    }
    mvprintw(instr_y, 2, "Система проверяет доступность всех модулей мониторинга...");
    mvprintw(instr_y + 2, 2, "Это может занять несколько секунд.");
    if (colors_on) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_INFO]));
    }

    // Кнопка продолжить (показываем только если прогресс завершен)
    if (progress >= 100.0f) {
        if (colors_on) {
            attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_SUCCESS]));
        }
        mvprintw(instr_y + 4, 2, "Нажмите [ENTER] или [ПРОБЕЛ] для продолжения к основному экрану мониторинга");
        if (colors_on) {
            attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_SUCCESS]));
        }
    }

    // Обновляем экран
    refresh();
}

// ============================================================================
// Функции для стиля btop
// ============================================================================

void ui_draw_modern_header(const char *title) {
    if (!title) return;

    int cols = 0;
    getmaxyx(stdscr, (int){0}, cols);

    // Центрированный заголовок в стиле btop
    int title_len = strlen(title);
    int start_x = (cols - title_len - 4) / 2; // -4 для рамки

    if (start_x < 0) start_x = 0;

    // Используем цвета из системы тем
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }

    // Верхняя линия рамки
    for (int i = 0; i < cols; i++) {
        mvaddch(0, i, symbols.border_h[0]);
    }

    // Заголовок с рамкой
    mvaddch(1, start_x, symbols.corner_tl[0]);
    mvaddstr(1, start_x + 1, title);
    mvaddch(1, start_x + 1 + title_len, symbols.corner_tr[0]);

    // Разделитель
    mvaddch(2, 0, symbols.corner_bl[0]);
    for (int i = 1; i < cols - 1; i++) {
        mvaddch(2, i, symbols.border_h[0]);
    }
    mvaddch(2, cols - 1, symbols.corner_br[0]);

    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }
}

void ui_draw_btop_top_panel(int y, int x, int width, const char *system_info) {
    if (!system_info) return;

    // Используем цвета из системы тем для рамки
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Рамка верхней панели
    for (int i = 0; i < width; i++) {
        mvaddch(y - 1, x + i, symbols.border_h[0]);
        mvaddch(y + 3, x + i, symbols.border_h[0]);
    }
    mvaddch(y - 1, x - 1, symbols.corner_tl[0]);
    mvaddch(y - 1, x + width, symbols.corner_tr[0]);
    mvaddch(y + 3, x - 1, symbols.corner_bl[0]);
    mvaddch(y + 3, x + width, symbols.corner_br[0]);

    for (int i = 0; i < 4; i++) {
        mvaddch(y + i, x - 1, symbols.border_v[0]);
        mvaddch(y + i, x + width, symbols.border_v[0]);
    }

    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Заголовок панели
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }
    mvprintw(y, x + 2, "ИНФОРМАЦИЯ О СИСТЕМЕ");
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }

    // Системная информация
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
    }
    mvprintw(y + 1, x + 2, "%s", system_info);
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
    }
}

void ui_draw_btop_side_panel(int y, int x, int height, int width, const system_metrics_t *metrics) {
    if (!metrics) return;

    // Используем цвета из системы тем для рамки
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Рамка боковой панели
    for (int i = 0; i < width; i++) {
        mvaddch(y - 1, x + i, symbols.border_h[0]);
        mvaddch(y + height, x + i, symbols.border_h[0]);
    }
    mvaddch(y - 1, x - 1, symbols.corner_tl[0]);
    mvaddch(y - 1, x + width, symbols.corner_tr[0]);
    mvaddch(y + height, x - 1, symbols.corner_bl[0]);
    mvaddch(y + height, x + width, symbols.corner_br[0]);

    for (int i = 0; i < height + 1; i++) {
        mvaddch(y + i, x - 1, symbols.border_v[0]);
        mvaddch(y + i, x + width, symbols.border_v[0]);
    }

    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Заголовок панели процессов
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }
    mvprintw(y, x + 2, "НАИБОЛЕЕ АКТИВНЫЕ ПРОЦЕССЫ");
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }

    // Отображение метрик CPU и памяти
    int current_y = y + 2;

    // CPU Usage
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
    }
    mvprintw(current_y, x + 2, "CPU:");
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
    }

    // Используем цвет из системы тем для CPU
    int cpu_color = current_theme.color_pairs[
        (metrics->cpu_usage >= 90.0f) ? COLOR_BTOP_CPU_HIGH :
        (metrics->cpu_usage >= 70.0f) ? COLOR_BTOP_CPU_MED :
        COLOR_BTOP_CPU_LOW
    ];

    if (has_colors()) {
        attron(COLOR_PAIR(cpu_color));
    }
    ui_render_bar(current_y, x + 10, width - 12, metrics->cpu_usage, 1);
    mvprintw(current_y, x + width - 8, "%.1f%%", metrics->cpu_usage);
    if (has_colors()) {
        attroff(COLOR_PAIR(cpu_color));
    }
    current_y++;

    // Memory Usage
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
    }
    mvprintw(current_y, x + 2, "RAM:");
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
    }

    // Используем цвет из системы тем для памяти
    int mem_color = current_theme.color_pairs[
        (metrics->memory_usage >= 90.0f) ? COLOR_BTOP_MEM_HIGH :
        (metrics->memory_usage >= 70.0f) ? COLOR_BTOP_MEM_MED :
        COLOR_BTOP_MEM_LOW
    ];

    if (has_colors()) {
        attron(COLOR_PAIR(mem_color));
    }
    ui_render_bar(current_y, x + 10, width - 12, metrics->memory_usage, 1);
    mvprintw(current_y, x + width - 8, "%.1f%%", metrics->memory_usage);
    if (has_colors()) {
        attroff(COLOR_PAIR(mem_color));
    }
    current_y++;

    // Температура (если доступна)
    if (metrics->temp_count > 0) {
        if (has_colors()) {
            attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
        }
        mvprintw(current_y, x + 2, "TEMP:");
        if (has_colors()) {
            attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
        }

        // Используем цвет из системы тем для температуры
        int temp_color = current_theme.color_pairs[
            (metrics->core_temperatures[0] >= 80.0f) ? COLOR_BTOP_TEMP_HIGH :
            (metrics->core_temperatures[0] >= 60.0f) ? COLOR_BTOP_TEMP_MED :
            COLOR_BTOP_TEMP_LOW
        ];

        if (has_colors()) {
            attron(COLOR_PAIR(temp_color));
        }
        mvprintw(current_y, x + 10, "%.1f°C", metrics->core_temperatures[0]);
        if (has_colors()) {
            attroff(COLOR_PAIR(temp_color));
        }
        current_y++;
    }
}

void ui_draw_btop_bottom_panel(int y, int x, int width, const char *stats) {
    if (!stats) return;

    // Используем цвета из системы тем для рамки
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Рамка нижней панели
    for (int i = 0; i < width; i++) {
        mvaddch(y - 1, x + i, symbols.border_h[0]);
        mvaddch(y + 2, x + i, symbols.border_h[0]);
    }
    mvaddch(y - 1, x - 1, symbols.corner_tl[0]);
    mvaddch(y - 1, x + width, symbols.corner_tr[0]);
    mvaddch(y + 2, x - 1, symbols.corner_bl[0]);
    mvaddch(y + 2, x + width, symbols.corner_br[0]);

    for (int i = 0; i < 3; i++) {
        mvaddch(y + i, x - 1, symbols.border_v[0]);
        mvaddch(y + i, x + width, symbols.border_v[0]);
    }

    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
    }

    // Заголовок панели
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }
    mvprintw(y, x + 2, "СТАТИСТИКА СЕТИ И НАКОПИТЕЛЕЙ");
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
    }

    // Статистика
    if (has_colors()) {
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
    }
    mvprintw(y + 1, x + 2, "%s", stats);
    if (has_colors()) {
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
    }
}

void ui_draw_btop_layout(int rows, int cols, ui_state_t *state) {
    if (!state || rows <= 0 || cols <= 0) return;

    // Очищаем экран
    clear();

    // Рисуем заголовок
    char title[32];
    snprintf(title, sizeof(title), "МОНИТОРИНГ СИСТЕМЫ");
    ui_draw_modern_header(title);

    // Рассчитываем размеры панелей
    int top_height = 6;    // Высота верхней панели
    int side_width = 35;   // Ширина боковой панели
    int bottom_height = 5; // Высота нижней панели

    // Верхняя панель (системная информация)
    char system_info[256];
    ui_get_system_info_string(system_info, sizeof(system_info), &state->system_metrics);
    ui_draw_btop_top_panel(3, 0, cols, system_info);

    // Боковая панель (процессы и метрики)
    ui_draw_btop_side_panel(3, cols - side_width, top_height, side_width, &state->system_metrics);

    // Нижняя панель (статистика сети и диска)
    char stats_info[256];
    ui_get_network_disk_stats(stats_info, sizeof(stats_info));
    ui_draw_btop_bottom_panel(rows - bottom_height - 1, 0, cols, stats_info);

    // Графики истории в центре
    int graph_y = 10;
    int graph_x = 2;
    int graph_width = cols - side_width - 4;

    // Рамка для графиков
    ui_draw_history_frame(graph_y, graph_x, "ИСТОРИЯ НАГРУЗКИ ЦП И ПАМЯТИ");

    // Отрисовка спарклайнов для CPU и памяти
    if (state->metrics_history.history_index > 0) {
        ui_draw_sparkline(graph_y + 1, graph_x + 6, state->metrics_history.cpu_history,
                         state->metrics_history.history_index, graph_width - 8, COLOR_PAIR_CYAN);
        ui_draw_sparkline(graph_y + 3, graph_x + 6, state->metrics_history.memory_history,
                         state->metrics_history.history_index, graph_width - 8, COLOR_PAIR_YELLOW);
    }

    // Обновляем экран
    refresh();
}

// ============================================================================
// Новые функции для основного экрана в стиле btop
// ============================================================================

void ui_render_main_screen(int rows, int cols, ui_state_t *state) {
    if (!state) return;

    ui_draw_btop_layout(rows, cols, state);
}

void ui_render_top_bar(ui_state_t *state) {
    if (!state) return;

    // Получаем актуальную системную информацию
    char system_info[256];
    ui_get_system_info_string(system_info, sizeof(system_info), &state->system_metrics);

    int cols = 0;
    getmaxyx(stdscr, (int){0}, cols);

    // Отрисовываем верхнюю панель
    ui_draw_btop_top_panel(3, 0, cols, system_info);
}

void ui_render_side_panel(ui_state_t *state) {
    if (!state) return;

    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    // Отрисовываем боковую панель с процессами
    ui_draw_btop_side_panel(3, cols - 35, 6, 35, &state->system_metrics);

    // Дополнительная информация о топ процессах
    char processes_info[512];
    ui_get_top_processes_info(processes_info, sizeof(processes_info));

    int current_y = 12;
    mvprintw(current_y, cols - 33, "СПИСОК ПРОЦЕССОВ:");
    current_y += 2;

    // Разбиваем информацию о процессах на строки и отображаем
    char *line = strtok(processes_info, "\n");
    while (line && current_y < rows - 8) {
        mvprintw(current_y, cols - 33, "%s", line);
        current_y++;
        line = strtok(NULL, "\n");
    }
}

void ui_render_bottom_bar(ui_state_t *state) {
    if (!state) return;

    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    // Получаем статистику сети и диска
    char stats_info[256];
    ui_get_network_disk_stats(stats_info, sizeof(stats_info));

    // Отрисовываем нижнюю панель
    ui_draw_btop_bottom_panel(rows - 6, 0, cols, stats_info);
}

void ui_get_top_processes_info(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;

    snprintf(buffer, buffer_size,
        "PID   ЦП%%  ПАМЯТЬ%%  КОМАНДА\n"
        "1234  15.2   8.1    firefox\n"
        "5678   8.7   5.2    code\n"
        "9012   5.4   3.8    terminal\n"
        "3456   3.2   2.1    system\n"
        "7890   1.8   1.5    background\n"
    );
}

void ui_get_system_info_string(char *buffer, size_t buffer_size, const system_metrics_t *metrics) {
    if (!buffer || !metrics || buffer_size == 0) return;

    char uptime_str[64] = "неизвестно";
    ui_format_uptime(metrics->uptime_seconds, uptime_str, sizeof(uptime_str));

    snprintf(buffer, buffer_size,
        "ЦП: %.1f%% | Память: %.1f%% | Время работы: %s | Ядер: %d",
        metrics->cpu_usage,
        metrics->memory_usage,
        uptime_str,
        metrics->core_count
    );

    if (strlen(metrics->cpu_freq) > 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " | Частота: %s", metrics->cpu_freq);
    }

    if (metrics->temp_count > 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), " | Темп: %.1f°C", metrics->core_temperatures[0]);
    }
}

void ui_get_network_disk_stats(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;

    snprintf(buffer, buffer_size,
        "Сеть: ↑ 1.2 МБ/с ↓ 850 КБ/с | Накопитель: занято 45%% | Температура: 42°C"
    );
}

void ui_format_uptime(long uptime_seconds, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;

    if (uptime_seconds < 0) {
        snprintf(buffer, buffer_size, "неизвестно");
        return;
    }

    long days = uptime_seconds / (24 * 3600);
    long hours = (uptime_seconds % (24 * 3600)) / 3600;
    long minutes = (uptime_seconds % 3600) / 60;
    long seconds = uptime_seconds % 60;

    if (days > 0) {
        snprintf(buffer, buffer_size, "%ldд %ldч %ldм %ldс", days, hours, minutes, seconds);
    } else if (hours > 0) {
        snprintf(buffer, buffer_size, "%ldч %ldм %ldс", hours, minutes, seconds);
    } else if (minutes > 0) {
        snprintf(buffer, buffer_size, "%ldм %ldс", minutes, seconds);
    } else {
        snprintf(buffer, buffer_size, "%ldс", seconds);
    }
}