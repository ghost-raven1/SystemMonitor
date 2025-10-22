/**
 * UI Window Manager Module
 *
 * Координирует работу между модулями рендеринга, обработки ввода и визуализации данных.
 * Управляет навигацией между экранами и общим состоянием интерфейса.
 */

#ifndef UI_WINDOW_MANAGER_H
#define UI_WINDOW_MANAGER_H

#include "ui_renderer.h"
#include "ui_input_handler.h"
#include "ui_data_visualizer.h"
#include <ncurses.h>

// Структура главного состояния UI
typedef struct {
    screen_type_t current_screen;
    input_state_t *input_state;
    system_metrics_t system_metrics;
    metrics_history_t metrics_history;
    int colors_enabled;
    int running;
    int needs_redraw;
} ui_state_t;

// Коллбэки для действий между модулями
typedef void (*screen_change_callback_t)(screen_type_t new_screen, void *user_data);
typedef void (*data_update_callback_t)(system_metrics_t *metrics, void *user_data);

/**
 * Инициализация оконного менеджера
 * @param colors_enabled - включить ли цветовое оформление
 * @return указатель на состояние UI или NULL при ошибке
 */
ui_state_t *ui_window_manager_init(int colors_enabled);

/**
 * Деинициализация оконного менеджера
 * @param state - указатель на состояние UI
 */
void ui_window_manager_cleanup(ui_state_t *state);

/**
 * Главный цикл обработки UI
 * @param state - состояние UI
 * @param screen_cb - коллбэк смены экрана
 * @param data_cb - коллбэк обновления данных
 * @param user_data - пользовательские данные для коллбэков
 */
void ui_run_main_loop(ui_state_t *state,
                     screen_change_callback_t screen_cb,
                     data_update_callback_t data_cb,
                     void *user_data);

/**
 * Переход к указанному экрану
 * @param state - состояние UI
 * @param screen - тип экрана для отображения
 */
void ui_switch_to_screen(ui_state_t *state, screen_type_t screen);

/**
 * Обновление системных метрик в состоянии UI
 * @param state - состояние UI
 */
void ui_update_system_metrics(ui_state_t *state);

/**
 * Обработка ввода пользователя
 * @param state - состояние UI
 * @param ch - код клавиши
 * @return 1 если продолжить обработку, 0 если выйти
 */
int ui_handle_user_input(ui_state_t *state, int ch);

/**
 * Отрисовка текущего экрана
 * @param state - состояние UI
 */
void ui_render_current_screen(ui_state_t *state);

/**
 * Установка коллбэка для смены экрана
 * @param state - состояние UI
 * @param callback - функция коллбэка
 * @param user_data - пользовательские данные
 */
void ui_set_screen_change_callback(ui_state_t *state,
                                  screen_change_callback_t callback,
                                  void *user_data);

/**
 * Установка коллбэка для обновления данных
 * @param state - состояние UI
 * @param callback - функция коллбэка
 * @param user_data - пользовательские данные
 */
void ui_set_data_update_callback(ui_state_t *state,
                                data_update_callback_t callback,
                                void *user_data);

/**
 * Получение текущего экрана
 * @param state - состояние UI
 * @return тип текущего экрана
 */
screen_type_t ui_get_current_screen(ui_state_t *state);

/**
 * Проверка необходимости перерисовки
 * @param state - состояние UI
 * @return 1 если нужна перерисовка, 0 иначе
 */
int ui_needs_redraw(ui_state_t *state);

/**
 * Установка флага необходимости перерисовки
 * @param state - состояние UI
 * @param needs_redraw - флаг необходимости перерисовки
 */
void ui_set_needs_redraw(ui_state_t *state, int needs_redraw);

/**
 * Получение состояния ввода
 * @param state - состояние UI
 * @return указатель на состояние ввода
 */
input_state_t *ui_get_input_state(ui_state_t *state);

/**
 * Получение системных метрик из состояния UI
 * @param state - состояние UI
 * @return указатель на системные метрики
 */
system_metrics_t *ui_get_system_metrics_from_state(ui_state_t *state);

/**
 * Сохранение снимка текущего состояния
 * @param state - состояние UI
 * @param filename - имя файла для сохранения
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ui_save_snapshot(ui_state_t *state, const char *filename);

/**
 * Обработка изменения размера терминала
 * @param state - состояние UI
 */
void ui_handle_terminal_resize(ui_state_t *state);

/**
 * Проверка доступности модулей системы
 * @param state - состояние UI
 * @return битовый флаг доступности модулей
 */
int ui_check_module_availability(ui_state_t *state);

/**
 * @brief Основная функция рендеринга для главного цикла
 * @param state - состояние UI
 * @return 0 при успехе, -1 при ошибке
 */
int ui_window_manager_render(ui_state_t *state);

#endif // UI_WINDOW_MANAGER_H