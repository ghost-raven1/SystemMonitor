/**
 * ui_theme.h - Современная система тем для терминального интерфейса в стиле btop
 *
 * Возможности:
 * - Расширенная цветовая палитра (16+ цветов)
 * - Градиентные progress-бары
 * - Анимированные элементы
 * - Адаптивная типографика
 * - Темы интерфейса (dark/light/neon/matrix)
 * - Unicode визуальные элементы
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include <ncurses.h>
#include "ui_data_visualizer.h"
#include "ui_state_manager.h"

// Цветовая палитра в стиле btop
typedef enum {
    // Базовые цвета
    COLOR_BTOP_BG = 16,      // Основной фон
    COLOR_BTOP_PANEL,        // Фон панелей
    COLOR_BTOP_BORDER,       // Границы
    COLOR_BTOP_TITLE,        // Заголовки
    COLOR_BTOP_TEXT,         // Основной текст
    COLOR_BTOP_TEXT_DIM,     // Приглушенный текст

    // Цвета для метрик
    COLOR_BTOP_CPU_HIGH,     // Высокая нагрузка CPU
    COLOR_BTOP_CPU_MED,      // Средняя нагрузка CPU
    COLOR_BTOP_CPU_LOW,      // Низкая нагрузка CPU

    COLOR_BTOP_MEM_HIGH,     // Высокая нагрузка памяти
    COLOR_BTOP_MEM_MED,      // Средняя нагрузка памяти
    COLOR_BTOP_MEM_LOW,      // Низкая нагрузка памяти

    COLOR_BTOP_TEMP_HIGH,    // Высокая температура
    COLOR_BTOP_TEMP_MED,     // Средняя температура
    COLOR_BTOP_TEMP_LOW,     // Низкая температура

    COLOR_BTOP_NET_HIGH,     // Высокая сетввая активность
    COLOR_BTOP_NET_MED,      // Средняя сетввая активность
    COLOR_BTOP_NET_LOW,      // Низкая сетввая активность

    // Акцентные цвета
    COLOR_BTOP_ACCENT1,      // Акцент 1 (голубой)
    COLOR_BTOP_ACCENT2,      // Акцент 2 (зеленый)
    COLOR_BTOP_ACCENT3,      // Акцент 3 (оранжевый)
    COLOR_BTOP_ACCENT4,      // Акцент 4 (фиолетовый)

    // Статусные цвета
    COLOR_BTOP_SUCCESS,      // Успех
    COLOR_BTOP_WARNING,      // Предупреждение
    COLOR_BTOP_ERROR,        // Ошибка
    COLOR_BTOP_INFO,         // Информация

    // Градиентные цвета для баров
    COLOR_BTOP_GRAD1,        // Градиент начало
    COLOR_BTOP_GRAD2,
    COLOR_BTOP_GRAD3,
    COLOR_BTOP_GRAD4,
    COLOR_BTOP_GRAD5,        // Градиент конец

    COLOR_BTOP_MAX
} btop_colors_t;

// Структура темы интерфейса
typedef struct {
    char name[32];           // Название темы
    short color_pairs[COLOR_BTOP_MAX];
    bool gradient_bars;      // Использовать градиентные бары
    bool animations;         // Включить анимации
    bool unicode_icons;      // Использовать Unicode иконки
    int refresh_rate;        // Частота обновления (мс)
} ui_theme_t;

// Типы тем интерфейса (определены в ui_renderer.h)

// Символы для визуальных элементов
typedef struct {
    // Границы и рамки
    const char* border_h;      // Горизонтальная линия
    const char* border_v;      // Вертикальная линия
    const char* corner_tl;     // Левый верхний угол
    const char* corner_tr;     // Правый верхний угол
    const char* corner_bl;     // Левый нижний угол
    const char* corner_br;     // Правый нижний угол

    // Progress бары
    const char* bar_full;      // Полный блок
    const char* bar_partial1;  // Частичный блок 1/4
    const char* bar_partial2;  // Частичный блок 2/4
    const char* bar_partial3;  // Частичный блок 3/4
    const char* bar_empty;     // Пустой блок

    // Спецсимволы
    const char* arrow_right;   // Стрелка вправо
    const char* arrow_left;    // Стрелка влево
    const char* arrow_up;      // Стрелка вверх
    const char* arrow_down;    // Стрелка вниз

    // Иконки
    const char* icon_cpu;      // Иконка CPU
    const char* icon_mem;      // Иконка памяти
    const char* icon_disk;     // Иконка диска
    const char* icon_net;      // Иконка сети
    const char* icon_temp;     // Иконка температуры
    const char* icon_proc;     // Иконка процессов
    const char* icon_gpu;      // Иконка GPU
    const char* icon_battery;  // Иконка батареи

    // Графические элементы
    const char* graph_full;    // Полный блок графика
    const char* graph_half;    // Половина блока графика
    const char* graph_low;     // Низкий уровень графика

} ui_symbols_t;

// Глобальные переменные темы
extern ui_theme_t current_theme;
extern ui_symbols_t symbols;

// Функции инициализации тем
int ui_theme_init(void);
void ui_theme_set_theme(theme_type_t theme);
void ui_theme_detect_terminal_colors(void);

// Функции цветового дизайна
void ui_draw_modern_box(int y, int x, int height, int width);
void ui_draw_gradient_bar(int y, int x, int width, float percent, int base_color);
void ui_draw_animated_bar(int y, int x, int width, float percent, int base_color);

// Функции типографики
void ui_print_header(const char* title);
void ui_print_metric(const char* label, const char* value, int color_pair);
void ui_print_status(const char* status, int status_type);

// Функции компоновки
void ui_layout_calculate(int rows, int cols, int* panel_heights, int* panel_widths);
void ui_draw_responsive_grid(int panels_count);

// Анимации и эффекты
void ui_animate_value(int y, int x, float old_val, float new_val, int color);

// Unicode иконки
const char* ui_get_icon(const char* icon_name);
const char* ui_get_bar_char(float percent);

// Управление темами
void ui_theme_save_custom(const char* filename);
void ui_theme_load_custom(const char* filename);

// Функции для layout в стиле btop
void ui_draw_top_panel(int rows, int cols, const char* system_info);
void ui_draw_side_panel(int rows, int cols, const system_metrics_t* metrics);
void ui_draw_bottom_panel(int rows, int cols, const char* stats);
void ui_draw_btop_layout(int rows, int cols, ui_state_t* state);

// UI Theme Manager функции
void ui_theme_manager_init(void);
void ui_theme_manager_set_layout(theme_type_t theme, int layout_type);
int ui_theme_manager_get_panel_heights(int rows, int* top_h, int* side_w, int* bottom_h);

#endif // UI_THEME_H