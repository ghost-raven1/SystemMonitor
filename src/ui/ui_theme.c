/**
 * ui_theme.c - Реализация современной системы тем для терминального интерфейса
 */

#include "ui/ui_theme.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

// Глобальные переменные темы
ui_theme_t current_theme;
ui_symbols_t symbols;

// Символы для визуальных элементов (Unicode)
static const ui_symbols_t modern_symbols = {
    // Границы и рамки
    .border_h = "─",
    .border_v = "│",
    .corner_tl = "┌",
    .corner_tr = "┐",
    .corner_bl = "└",
    .corner_br = "┘",

    // Progress бары
    .bar_full = "█",
    .bar_partial1 = "▏",
    .bar_partial2 = "▎",
    .bar_partial3 = "▍",
    .bar_empty = "░",

    // Стрелки
    .arrow_right = "→",
    .arrow_left = "←",
    .arrow_up = "↑",
    .arrow_down = "↓",

    // Иконки
    .icon_cpu = "🖥️",
    .icon_mem = "💾",
    .icon_disk = "💿",
    .icon_net = "🌐",
    .icon_temp = "🌡️",
    .icon_proc = "⚙️",
    .icon_gpu = "🎮",
    .icon_battery = "🔋",

    // Графические элементы
    .graph_full = "█",
    .graph_half = "▓",
    .graph_low = "▒"
};

// Фallback символы для старых терминалов
static const ui_symbols_t ascii_symbols = {
    // Границы и рамки
    .border_h = "-",
    .border_v = "|",
    .corner_tl = "+",
    .corner_tr = "+",
    .corner_bl = "+",
    .corner_br = "+",

    // Progress бары
    .bar_full = "#",
    .bar_partial1 = "#",
    .bar_partial2 = "#",
    .bar_partial3 = "#",
    .bar_empty = " ",

    // Стрелки
    .arrow_right = ">",
    .arrow_left = "<",
    .arrow_up = "^",
    .arrow_down = "v",

    // Иконки (ASCII версии)
    .icon_cpu = "[CPU]",
    .icon_mem = "[MEM]",
    .icon_disk = "[DISK]",
    .icon_net = "[NET]",
    .icon_temp = "[TEMP]",
    .icon_proc = "[PROC]",
    .icon_gpu = "[GPU]",
    .icon_battery = "[BAT]",

    // Графические элементы
    .graph_full = "#",
    .graph_half = "#",
    .graph_low = "."
};

// Темная тема в стиле btop
static const short dark_theme_colors[COLOR_BTOP_MAX] = {
    [COLOR_BTOP_BG] = COLOR_BLACK,
    [COLOR_BTOP_PANEL] = COLOR_BLACK,
    [COLOR_BTOP_BORDER] = COLOR_WHITE,
    [COLOR_BTOP_TITLE] = COLOR_CYAN,
    [COLOR_BTOP_TEXT] = COLOR_WHITE,
    [COLOR_BTOP_TEXT_DIM] = COLOR_WHITE,

    [COLOR_BTOP_CPU_HIGH] = COLOR_RED,
    [COLOR_BTOP_CPU_MED] = COLOR_YELLOW,
    [COLOR_BTOP_CPU_LOW] = COLOR_GREEN,

    [COLOR_BTOP_MEM_HIGH] = COLOR_MAGENTA,
    [COLOR_BTOP_MEM_MED] = COLOR_CYAN,
    [COLOR_BTOP_MEM_LOW] = COLOR_BLUE,

    [COLOR_BTOP_TEMP_HIGH] = COLOR_RED,
    [COLOR_BTOP_TEMP_MED] = COLOR_YELLOW,
    [COLOR_BTOP_TEMP_LOW] = COLOR_GREEN,

    [COLOR_BTOP_NET_HIGH] = COLOR_MAGENTA,
    [COLOR_BTOP_NET_MED] = COLOR_CYAN,
    [COLOR_BTOP_NET_LOW] = COLOR_BLUE,

    [COLOR_BTOP_ACCENT1] = COLOR_CYAN,
    [COLOR_BTOP_ACCENT2] = COLOR_GREEN,
    [COLOR_BTOP_ACCENT3] = COLOR_YELLOW,
    [COLOR_BTOP_ACCENT4] = COLOR_MAGENTA,

    [COLOR_BTOP_SUCCESS] = COLOR_GREEN,
    [COLOR_BTOP_WARNING] = COLOR_YELLOW,
    [COLOR_BTOP_ERROR] = COLOR_RED,
    [COLOR_BTOP_INFO] = COLOR_BLUE,

    [COLOR_BTOP_GRAD1] = COLOR_BLUE,
    [COLOR_BTOP_GRAD2] = COLOR_CYAN,
    [COLOR_BTOP_GRAD3] = COLOR_GREEN,
    [COLOR_BTOP_GRAD4] = COLOR_YELLOW,
    [COLOR_BTOP_GRAD5] = COLOR_RED,
};

// Неоновая тема
static const short neon_theme_colors[COLOR_BTOP_MAX] = {
    [COLOR_BTOP_BG] = COLOR_BLACK,
    [COLOR_BTOP_PANEL] = COLOR_BLACK,
    [COLOR_BTOP_BORDER] = COLOR_GREEN,
    [COLOR_BTOP_TITLE] = COLOR_GREEN,
    [COLOR_BTOP_TEXT] = COLOR_GREEN,
    [COLOR_BTOP_TEXT_DIM] = COLOR_GREEN,

    [COLOR_BTOP_CPU_HIGH] = COLOR_RED,
    [COLOR_BTOP_CPU_MED] = COLOR_YELLOW,
    [COLOR_BTOP_CPU_LOW] = COLOR_GREEN,

    [COLOR_BTOP_MEM_HIGH] = COLOR_MAGENTA,
    [COLOR_BTOP_MEM_MED] = COLOR_CYAN,
    [COLOR_BTOP_MEM_LOW] = COLOR_BLUE,

    [COLOR_BTOP_TEMP_HIGH] = COLOR_RED,
    [COLOR_BTOP_TEMP_MED] = COLOR_YELLOW,
    [COLOR_BTOP_TEMP_LOW] = COLOR_GREEN,

    [COLOR_BTOP_NET_HIGH] = COLOR_MAGENTA,
    [COLOR_BTOP_NET_MED] = COLOR_CYAN,
    [COLOR_BTOP_NET_LOW] = COLOR_BLUE,

    [COLOR_BTOP_ACCENT1] = COLOR_GREEN,
    [COLOR_BTOP_ACCENT2] = COLOR_CYAN,
    [COLOR_BTOP_ACCENT3] = COLOR_MAGENTA,
    [COLOR_BTOP_ACCENT4] = COLOR_YELLOW,

    [COLOR_BTOP_SUCCESS] = COLOR_GREEN,
    [COLOR_BTOP_WARNING] = COLOR_YELLOW,
    [COLOR_BTOP_ERROR] = COLOR_RED,
    [COLOR_BTOP_INFO] = COLOR_CYAN,

    [COLOR_BTOP_GRAD1] = COLOR_GREEN,
    [COLOR_BTOP_GRAD2] = COLOR_CYAN,
    [COLOR_BTOP_GRAD3] = COLOR_BLUE,
    [COLOR_BTOP_GRAD4] = COLOR_MAGENTA,
    [COLOR_BTOP_GRAD5] = COLOR_RED,
};

// Матричная тема
static const short matrix_theme_colors[COLOR_BTOP_MAX] = {
    [COLOR_BTOP_BG] = COLOR_BLACK,
    [COLOR_BTOP_PANEL] = COLOR_BLACK,
    [COLOR_BTOP_BORDER] = COLOR_GREEN,
    [COLOR_BTOP_TITLE] = COLOR_GREEN,
    [COLOR_BTOP_TEXT] = COLOR_GREEN,
    [COLOR_BTOP_TEXT_DIM] = COLOR_GREEN,

    [COLOR_BTOP_CPU_HIGH] = COLOR_GREEN,
    [COLOR_BTOP_CPU_MED] = COLOR_GREEN,
    [COLOR_BTOP_CPU_LOW] = COLOR_GREEN,

    [COLOR_BTOP_MEM_HIGH] = COLOR_GREEN,
    [COLOR_BTOP_MEM_MED] = COLOR_GREEN,
    [COLOR_BTOP_MEM_LOW] = COLOR_GREEN,

    [COLOR_BTOP_TEMP_HIGH] = COLOR_GREEN,
    [COLOR_BTOP_TEMP_MED] = COLOR_GREEN,
    [COLOR_BTOP_TEMP_LOW] = COLOR_GREEN,

    [COLOR_BTOP_NET_HIGH] = COLOR_GREEN,
    [COLOR_BTOP_NET_MED] = COLOR_GREEN,
    [COLOR_BTOP_NET_LOW] = COLOR_GREEN,

    [COLOR_BTOP_ACCENT1] = COLOR_GREEN,
    [COLOR_BTOP_ACCENT2] = COLOR_GREEN,
    [COLOR_BTOP_ACCENT3] = COLOR_GREEN,
    [COLOR_BTOP_ACCENT4] = COLOR_GREEN,

    [COLOR_BTOP_SUCCESS] = COLOR_GREEN,
    [COLOR_BTOP_WARNING] = COLOR_GREEN,
    [COLOR_BTOP_ERROR] = COLOR_GREEN,
    [COLOR_BTOP_INFO] = COLOR_GREEN,

    [COLOR_BTOP_GRAD1] = COLOR_GREEN,
    [COLOR_BTOP_GRAD2] = COLOR_GREEN,
    [COLOR_BTOP_GRAD3] = COLOR_GREEN,
    [COLOR_BTOP_GRAD4] = COLOR_GREEN,
    [COLOR_BTOP_GRAD5] = COLOR_GREEN,
};

// Светлая тема в стиле btop
static const short light_theme_colors[COLOR_BTOP_MAX] = {
    [COLOR_BTOP_BG] = COLOR_WHITE,
    [COLOR_BTOP_PANEL] = COLOR_WHITE,
    [COLOR_BTOP_BORDER] = COLOR_BLACK,
    [COLOR_BTOP_TITLE] = COLOR_BLUE,
    [COLOR_BTOP_TEXT] = COLOR_BLACK,
    [COLOR_BTOP_TEXT_DIM] = COLOR_BLACK,

    [COLOR_BTOP_CPU_HIGH] = COLOR_RED,
    [COLOR_BTOP_CPU_MED] = COLOR_YELLOW,
    [COLOR_BTOP_CPU_LOW] = COLOR_GREEN,

    [COLOR_BTOP_MEM_HIGH] = COLOR_MAGENTA,
    [COLOR_BTOP_MEM_MED] = COLOR_CYAN,
    [COLOR_BTOP_MEM_LOW] = COLOR_BLUE,

    [COLOR_BTOP_TEMP_HIGH] = COLOR_RED,
    [COLOR_BTOP_TEMP_MED] = COLOR_YELLOW,
    [COLOR_BTOP_TEMP_LOW] = COLOR_GREEN,

    [COLOR_BTOP_NET_HIGH] = COLOR_MAGENTA,
    [COLOR_BTOP_NET_MED] = COLOR_CYAN,
    [COLOR_BTOP_NET_LOW] = COLOR_BLUE,

    [COLOR_BTOP_ACCENT1] = COLOR_BLUE,
    [COLOR_BTOP_ACCENT2] = COLOR_GREEN,
    [COLOR_BTOP_ACCENT3] = COLOR_YELLOW,
    [COLOR_BTOP_ACCENT4] = COLOR_MAGENTA,

    [COLOR_BTOP_SUCCESS] = COLOR_GREEN,
    [COLOR_BTOP_WARNING] = COLOR_YELLOW,
    [COLOR_BTOP_ERROR] = COLOR_RED,
    [COLOR_BTOP_INFO] = COLOR_BLUE,

    [COLOR_BTOP_GRAD1] = COLOR_BLUE,
    [COLOR_BTOP_GRAD2] = COLOR_CYAN,
    [COLOR_BTOP_GRAD3] = COLOR_GREEN,
    [COLOR_BTOP_GRAD4] = COLOR_YELLOW,
    [COLOR_BTOP_GRAD5] = COLOR_RED,
};

// Инициализация системы тем
int ui_theme_init(void) {
    // Определяем поддержку Unicode в терминале
    const char* term = getenv("TERM");
    const char* lang = getenv("LANG");

    if (term && strstr(term, "xterm") && lang && strstr(lang, "UTF")) {
        memcpy(&symbols, &modern_symbols, sizeof(ui_symbols_t));
    } else {
        memcpy(&symbols, &ascii_symbols, sizeof(ui_symbols_t));
    }

    // Устанавливаем темную тему по умолчанию
    ui_theme_set_theme(THEME_BTOP_DARK);

    return 0;
}

// Определение цветовых пар для темы
static void init_color_pairs(const short* theme_colors) {
    if (!has_colors()) return;

    for (int i = 0; i < COLOR_BTOP_MAX; i++) {
        if (i < 16) { // Стандартные цвета
            current_theme.color_pairs[i] = i;
        } else { // Дополнительные цвета
            // Определяем цвет фона в зависимости от темы
            short bg_color = COLOR_BLACK; // По умолчанию темный фон

            // Для светлой темы используем белый фон
            if (theme_colors == light_theme_colors) {
                bg_color = COLOR_WHITE;
            }

            init_pair(i, theme_colors[i], bg_color);
            current_theme.color_pairs[i] = i;
        }
    }
}

// Получение текущей темы
theme_type_t ui_theme_get_current_theme(void) {
    if (strcmp(current_theme.name, "btop-dark") == 0) {
        return THEME_BTOP_DARK;
    } else if (strcmp(current_theme.name, "btop-neon") == 0) {
        return THEME_BTOP_NEON;
    } else if (strcmp(current_theme.name, "btop-matrix") == 0) {
        return THEME_BTOP_MATRIX;
    } else if (strcmp(current_theme.name, "btop-light") == 0) {
        return THEME_BTOP_LIGHT;
    }
    return THEME_BTOP_DARK; // Fallback
}

// Получение названия текущей темы
const char* ui_theme_get_current_theme_name(void) {
    return current_theme.name;
}

// Установка темы интерфейса
void ui_theme_set_theme(theme_type_t theme) {
    switch (theme) {
        case THEME_BTOP_DARK:
            strcpy(current_theme.name, "btop-dark");
            init_color_pairs(dark_theme_colors);
            current_theme.gradient_bars = true;
            current_theme.animations = true;
            current_theme.unicode_icons = true;
            current_theme.refresh_rate = 2000;
            break;

        case THEME_BTOP_NEON:
            strcpy(current_theme.name, "btop-neon");
            init_color_pairs(neon_theme_colors);
            current_theme.gradient_bars = true;
            current_theme.animations = true;
            current_theme.unicode_icons = true;
            current_theme.refresh_rate = 1500;
            break;

        case THEME_BTOP_MATRIX:
            strcpy(current_theme.name, "btop-matrix");
            init_color_pairs(matrix_theme_colors);
            current_theme.gradient_bars = true;
            current_theme.animations = false; // Отключаем анимации для стиля matrix
            current_theme.unicode_icons = false; // ASCII only для аутентичности
            current_theme.refresh_rate = 3000;
            break;

        case THEME_BTOP_LIGHT:
            strcpy(current_theme.name, "btop-light");
            init_color_pairs(light_theme_colors);
            current_theme.gradient_bars = true;
            current_theme.animations = true;
            current_theme.unicode_icons = true;
            current_theme.refresh_rate = 2000;
            break;

        default:
            ui_theme_set_theme(THEME_BTOP_DARK);
            break;
    }
}

// Определение поддержки Unicode в терминале
static bool ui_theme_detect_unicode_support(void) {
    const char* term = getenv("TERM");
    const char* lang = getenv("LANG");
    const char* lc_all = getenv("LC_ALL");
    const char* lc_ctype = getenv("LC_CTYPE");

    // Проверяем переменные окружения на поддержку UTF-8
    if (lang && strstr(lang, "UTF-8")) return true;
    if (lc_all && strstr(lc_all, "UTF-8")) return true;
    if (lc_ctype && strstr(lc_ctype, "UTF-8")) return true;

    // Проверяем тип терминала
    if (term) {
        if (strstr(term, "xterm") || strstr(term, "rxvt") ||
            strstr(term, "konsole") || strstr(term, "gnome") ||
            strstr(term, "mate") || strstr(term, "xfce") ||
            strstr(term, "alacritty") || strstr(term, "kitty")) {
            return true;
        }
    }

    // Проверяем локаль через setlocale
    char* current_locale = setlocale(LC_CTYPE, NULL);
    if (current_locale && strstr(current_locale, "UTF-8")) {
        return true;
    }

    return false;
}

// Определение возможностей терминала
void ui_theme_detect_terminal_colors(void) {
    if (has_colors()) {
        start_color();

        // Определяем поддержку 256 цветов
        if (COLORS >= 256) {
            // Используем расширенную палитру
            current_theme.gradient_bars = true;
        } else {
            // Ограничиваемся базовыми цветами
            current_theme.gradient_bars = false;
        }
    } else {
        current_theme.gradient_bars = false;
        current_theme.animations = false;
    }

    // Определяем поддержку Unicode
    current_theme.unicode_icons = ui_theme_detect_unicode_support();
}



// Анимированный бар (простая анимация)
void ui_draw_animated_bar(int y, int x, int width, float percent, int base_color) {
    static int animation_frame = 0;
    animation_frame = (animation_frame + 1) % 4;

    ui_draw_gradient_bar(y, x, width, percent, base_color);

    // Добавляем анимированные элементы для высоких значений
    if (percent > 80 && current_theme.animations) {
        attron(COLOR_PAIR(COLOR_BTOP_ACCENT1));
        if (animation_frame == 0) {
            mvaddch(y, x + width - 1, '!');
        } else if (animation_frame == 1) {
            mvaddch(y, x + width - 1, '?');
        } else if (animation_frame == 2) {
            mvaddch(y, x + width - 1, '*');
        } else {
            mvaddch(y, x + width - 1, '!');
        }
        attroff(COLOR_PAIR(COLOR_BTOP_ACCENT1));
    }
}

// Печать заголовка в стиле btop
void ui_print_header(const char* title) {
    if (!has_colors() || !title) return;

    attron(COLOR_PAIR(COLOR_BTOP_TITLE) | A_BOLD);
    printw("%s", title);
    attroff(COLOR_PAIR(COLOR_BTOP_TITLE) | A_BOLD);
}

// Печать метрики с иконкой
void ui_print_metric(const char* label, const char* value, int color_pair) {
    if (!label || !value) return;

    attron(COLOR_PAIR(COLOR_BTOP_TEXT_DIM));
    printw("%s", label);
    attroff(COLOR_PAIR(COLOR_BTOP_TEXT_DIM));

    attron(COLOR_PAIR(color_pair));
    printw("%s", value);
    attroff(COLOR_PAIR(color_pair));

    printw(" ");
}

// Печать статуса с цветовой индикацией
void ui_print_status(const char* status, int status_type) {
    if (!status) return;

    int color = COLOR_BTOP_TEXT;
    switch (status_type) {
        case 0: color = COLOR_BTOP_SUCCESS; break; // Success
        case 1: color = COLOR_BTOP_WARNING; break; // Warning
        case 2: color = COLOR_BTOP_ERROR; break;   // Error
        case 3: color = COLOR_BTOP_INFO; break;    // Info
        default: color = COLOR_BTOP_TEXT; break;
    }

    attron(COLOR_PAIR(color));
    printw("%s", status);
    attroff(COLOR_PAIR(color));
}

// Получение иконки по имени
const char* ui_get_icon(const char* icon_name) {
    if (!current_theme.unicode_icons) {
        // Возвращаем ASCII версии для простоты
        if (strcmp(icon_name, "cpu") == 0) return symbols.icon_cpu;
        if (strcmp(icon_name, "mem") == 0) return symbols.icon_mem;
        if (strcmp(icon_name, "disk") == 0) return symbols.icon_disk;
        if (strcmp(icon_name, "net") == 0) return symbols.icon_net;
        if (strcmp(icon_name, "temp") == 0) return symbols.icon_temp;
        if (strcmp(icon_name, "proc") == 0) return symbols.icon_proc;
        if (strcmp(icon_name, "gpu") == 0) return symbols.icon_gpu;
        if (strcmp(icon_name, "battery") == 0) return symbols.icon_battery;
    }

    return symbols.icon_cpu; // Fallback
}

// Получение символа бара по проценту
const char* ui_get_bar_char(float percent) {
    if (percent >= 100) return symbols.bar_full;
    if (percent >= 75) return symbols.bar_partial3;
    if (percent >= 50) return symbols.bar_partial2;
    if (percent >= 25) return symbols.bar_partial1;
    return symbols.bar_empty;
}

// Анимация значения (простая реализация)
void ui_animate_value(int y, int x, float old_val, float new_val, int color) {
    if (!current_theme.animations) {
        attron(COLOR_PAIR(color));
        mvprintw(y, x, "%.1f%%", new_val);
        attroff(COLOR_PAIR(color));
        return;
    }

    // Простая анимация - показываем изменение цвета
    if (new_val > old_val + 5) {
        attron(COLOR_PAIR(COLOR_BTOP_SUCCESS));
        mvprintw(y, x, "↗ %.1f%%", new_val);
        attroff(COLOR_PAIR(COLOR_BTOP_SUCCESS));
    } else if (new_val < old_val - 5) {
        attron(COLOR_PAIR(COLOR_BTOP_ERROR));
        mvprintw(y, x, "↘ %.1f%%", new_val);
        attroff(COLOR_PAIR(COLOR_BTOP_ERROR));
    } else {
        attron(COLOR_PAIR(color));
        mvprintw(y, x, "→ %.1f%%", new_val);
        attroff(COLOR_PAIR(color));
    }
}


// Расчет компоновки для адаптивной сетки
void ui_layout_calculate(int rows, int cols, int* panel_heights, int* panel_widths) {
    if (!panel_heights || !panel_widths) return;

    // Адаптивная компоновка в зависимости от размера терминала
    if (cols >= 120) {
        // Широкий терминал - 2 колонки
        panel_widths[0] = cols / 2 - 2;
        panel_widths[1] = cols / 2 - 2;
        panel_heights[0] = rows / 3;
        panel_heights[1] = rows / 3;
        panel_heights[2] = rows / 3;
    } else if (cols >= 80) {
        // Средний терминал - 2 колонки меньшего размера
        panel_widths[0] = cols / 2 - 1;
        panel_widths[1] = cols / 2 - 1;
        panel_heights[0] = rows / 2;
        panel_heights[1] = rows / 2;
    } else {
        // Узкий терминал - 1 колонка
        panel_widths[0] = cols - 4;
        panel_widths[1] = cols - 4;
        panel_heights[0] = rows / 3;
        panel_heights[1] = rows / 3;
        panel_heights[2] = rows / 3;
    }
}

// Отрисовка современной рамки в стиле btop
void ui_draw_modern_box(int y, int x, int height, int width) {
    if (height <= 0 || width <= 0) return;

    int color_pair = COLOR_PAIR(COLOR_BTOP_BORDER);

    // Используем цветовую пару границы
    attron(color_pair);

    // Верхняя граница
    mvprintw(y, x, "%s", symbols.corner_tl);
    for (int i = 1; i < width - 1; i++) {
        mvprintw(y, x + i, "%s", symbols.border_h);
    }
    mvprintw(y, x + width - 1, "%s", symbols.corner_tr);

    // Боковые границы
    for (int i = 1; i < height - 1; i++) {
        mvprintw(y + i, x, "%s", symbols.border_v);
        mvprintw(y + i, x + width - 1, "%s", symbols.border_v);
    }

    // Нижняя граница
    mvprintw(y + height - 1, x, "%s", symbols.corner_bl);
    for (int i = 1; i < width - 1; i++) {
        mvprintw(y + height - 1, x + i, "%s", symbols.border_h);
    }
    mvprintw(y + height - 1, x + width - 1, "%s", symbols.corner_br);

    attroff(color_pair);
}

// Отрисовка адаптивной сетки панелей
void ui_draw_responsive_grid(int panels_count) {
    if (panels_count <= 0) return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int panel_heights[panels_count];
    int panel_widths[panels_count];

    ui_layout_calculate(rows, cols, panel_heights, panel_widths);

    // Отрисовываем рамки для каждой панели
    int current_y = 1;
    for (int i = 0; i < panels_count && current_y < rows - 2; i++) {
        int height = panel_heights[i] > 0 ? panel_heights[i] : 5; // Минимум 5 строк
        if (current_y + height >= rows - 2) height = rows - current_y - 2;

        ui_draw_modern_box(current_y, 1, height, cols - 2);
        current_y += height + 1; // +1 для отступа между панелями
    }
}

// Градиентный бар с улучшенной визуализацией
void ui_draw_gradient_bar(int y, int x, int width, float percent, int base_color) {
    if (width <= 0) return;

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    int filled = (int)((percent / 100.0f) * width);

    // Используем цветовую схему темы или базовый цвет
    int color_pair = base_color > 0 ? base_color :
                     (percent >= 90 ? COLOR_BTOP_CPU_HIGH :
                      percent >= 75 ? COLOR_BTOP_CPU_MED :
                      COLOR_BTOP_CPU_LOW);

    attron(COLOR_PAIR(color_pair));

    for (int i = 0; i < width; i++) {
        char fill_char = symbols.bar_empty[0];
        if (i < filled) {
            // Градиент: разные символы для заполнения
            if (percent >= 95.0f) fill_char = symbols.bar_full[0];
            else if (percent >= 85.0f) fill_char = symbols.bar_partial3[0];
            else if (percent >= 70.0f) fill_char = symbols.bar_partial2[0];
            else if (percent >= 50.0f) fill_char = symbols.bar_partial1[0];
            else fill_char = symbols.bar_full[0];
        }
        mvaddch(y, x + i, fill_char);
    }

    attroff(COLOR_PAIR(color_pair));
}


// Применение темы ко всему экрану
void ui_theme_apply_to_screen(void) {
    // Очищаем экран для полной перерисовки с новой темой
    clear();

    // Обновляем цветовые пары для всей темы
    if (has_colors()) {
        for (int i = 0; i < COLOR_BTOP_MAX; i++) {
            if (i >= 16) { // Дополнительные цвета
                // Определяем цвет фона в зависимости от темы
                short bg_color = COLOR_BLACK;

                // Для светлой темы используем белый фон
                if (strcmp(current_theme.name, "btop-light") == 0) {
                    bg_color = COLOR_WHITE;
                }

                init_pair(i, current_theme.color_pairs[i], bg_color);
            }
        }
    }

    // Обновляем экран с новой темой
    refresh();
}

// Переключение темы с анимацией перехода
void ui_theme_switch_theme(theme_type_t theme) {
    // Здесь можно добавить логику определения текущей темы

    // Применяем новую тему
    ui_theme_set_theme(theme);

    // Применяем тему ко всему интерфейсу
    ui_theme_apply_to_screen();

    // Здесь можно добавить анимацию перехода между темами
    // Например, плавное изменение цветов
}

// Сохранение пользовательской темы
void ui_theme_save_custom(const char* filename) {
    if (!filename) return;

    FILE* file = fopen(filename, "w");
    if (!file) return;

    // Сохраняем текущую тему в файл
    fprintf(file, "# Пользовательская тема SystemMonitor\n");
    fprintf(file, "theme_name=%s\n", current_theme.name);

    // Сохраняем цветовые пары
    for (int i = 0; i < COLOR_BTOP_MAX; i++) {
        if (i >= 16) { // Только дополнительные цвета
            fprintf(file, "color_%d=%d\n", i, current_theme.color_pairs[i]);
        }
    }

    // Сохраняем настройки темы
    fprintf(file, "gradient_bars=%d\n", current_theme.gradient_bars);
    fprintf(file, "animations=%d\n", current_theme.animations);
    fprintf(file, "unicode_icons=%d\n", current_theme.unicode_icons);
    fprintf(file, "refresh_rate=%d\n", current_theme.refresh_rate);

    fclose(file);
}

// Загрузка пользовательской темы
void ui_theme_load_custom(const char* filename) {
    if (!filename) return;

    FILE* file = fopen(filename, "r");
    if (!file) return;

    char line[256];
    char theme_name[32] = "";

    // Читаем настройки темы
    while (fgets(line, sizeof(line), file)) {
        // Убираем пробелы и переносы строк
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';

        // Парсим параметры
        if (strncmp(line, "theme_name=", 11) == 0) {
            strcpy(theme_name, line + 11);
        }
        // Здесь можно добавить парсинг других параметров
    }

    fclose(file);

    // Применяем загруженную тему
    if (strlen(theme_name) > 0) {
        strcpy(current_theme.name, theme_name);
        // Здесь можно восстановить другие настройки темы
        ui_theme_apply_to_screen();
    }
}