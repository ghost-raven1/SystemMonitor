/**
 * UI Renderer Module
 *
 * Отвечает за отрисовку графики, текста и визуальных элементов интерфейса.
 * Содержит чистые функции рендеринга без бизнес-логики.
 */

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <ncurses.h>
#include "ui_theme.h"

// Color pair constants
#define COLOR_PAIR_RED     1
#define COLOR_PAIR_GREEN   2
#define COLOR_PAIR_YELLOW  3
#define COLOR_PAIR_CYAN    4
#define COLOR_PAIR_WHITE   5
#define COLOR_PAIR_MAGENTA 6

/**
 * Инициализация модуля рендеринга
 * Должна вызываться после инициализации ncurses
 */
void ui_renderer_init(void);

/**
 * Деинициализация модуля рендеринга
 */
void ui_renderer_cleanup(void);

/**
 * Вывод текста с обрезкой по ширине терминала
 * @param y - строка для вывода
 * @param x - колонка для вывода
 * @param fmt - формат строки (как printf)
 * @param ... - аргументы формата
 */
void ui_printw_clip(int y, int x, const char *fmt, ...);

/**
 * Вывод цветного текста с обрезкой по ширине терминала
 * @param y - строка для вывода
 * @param x - колонка для вывода
 * @param color_pair - пара цветов ncurses
 * @param fmt - формат строки (как printf)
 * @param ... - аргументы формата
 */
void ui_printw_clip_color(int y, int x, int color_pair, const char *fmt, ...);

/**
 * Отрисовка горизонтального разделителя
 * @param y - строка для отрисовки
 * @param x - начальная колонка
 */
void ui_draw_hsep(int y, int x);

/**
 * Отрисовка полосы загрузки
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param width - ширина полосы
 * @param percent - процент заполнения (0-100)
 * @param colors_on - включить ли цветовое оформление
 */
void ui_render_bar(int y, int x, int width, float percent, int colors_on);

/**
 * Выбор цвета по процентному значению
 * @param percent - процент (0-100)
 * @param warn_threshold - порог предупреждения
 * @param crit_threshold - порог критичного значения
 * @return пара цветов ncurses
 */
int ui_choose_color_by_percent(float percent, float warn_threshold, float crit_threshold);

/**
 * Отрисовка градиентной полосы загрузки (улучшенная версия)
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param width - ширина полосы
 * @param percent - процент заполнения (0-100)
 * @param color_scheme - схема цветов
 */
void ui_draw_gradient_bar(int y, int x, int width, float percent, int color_scheme);

/**
 * Отрисовка современной рамки
 * @param start_y - начальная строка
 * @param start_x - начальная колонка
 * @param height - высота рамки
 * @param width - ширина рамки
 */
void ui_draw_modern_box(int start_y, int start_x, int height, int width);

/**
 * Отрисовка заголовка в стиле btop
 * @param title - текст заголовка
 */
void ui_draw_modern_header(const char *title);

/**
 * Отрисовка спарклайн-графика (мини-график)
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param values - массив значений (0-100)
 * @param count - количество значений
 * @param width - ширина графика
 * @param color_pair - пара цветов для отрисовки
 */
void ui_draw_sparkline(int y, int x, const float *values, int count, int width, int color_pair);

/**
 * Отрисовка текстовой метки с иконкой
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param icon - иконка (может быть NULL)
 * @param label - текст метки
 * @param value - значение для отображения
 * @param color_pair - пара цветов (0 - без цвета)
 */
void ui_draw_labeled_value(int y, int x, const char *icon, const char *label,
                          const char *value, int color_pair);

/**
 * Отрисовка статусной строки с индикаторами модулей
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param module_status - строка со статусом модулей
 */
void ui_draw_status_bar(int y, int x, const char *module_status);

/**
 * Отрисовка рамки для графика истории
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param title - заголовок графика
 */
void ui_draw_history_frame(int y, int x, const char *title);

#endif // UI_RENDERER_H