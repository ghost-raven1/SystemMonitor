/**
 * UI Controller Module
 *
 * Управление навигацией между экранами, обработкой ввода и координацией UI компонентов.
 * Реализует паттерн Controller в MVC.
 */

#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <ncurses.h>

// Определения клавиш
#define KEY_TAB 9
#define KEY_BTAB 353  // Shift+Tab

#include "ui_renderer.h"  // Для ui_state_t и screen_type_t
#include "ui_input_handler.h"
#include "ui_state_manager.h"
#include "ui_diagnostics.h"  // Для функций диагностики
#include "../core/app_context.h"

// Коллбэки для действий
typedef void (*ui_action_callback_t)(ui_state_t *state, void *user_data);

// Структура для горячих клавиш
typedef struct {
    int key;
    ui_action_callback_t callback;
    const char *description;
} hotkey_t;

// Функции инициализации
int ui_controller_init(ui_state_t *state, app_context_t *ctx);
void ui_controller_cleanup(void);

// Навигация
int ui_controller_switch_screen(ui_state_t *state, screen_type_t screen);
int ui_controller_next_screen(ui_state_t *state);
int ui_controller_prev_screen(ui_state_t *state);

// Обработка ввода
int ui_controller_handle_input(ui_state_t *state, int ch);
int ui_controller_process_hotkey(ui_state_t *state, int ch);

// Интеграция с Input Handler
int ui_controller_register_hotkeys(ui_state_t *state);
void ui_controller_set_navigation_callbacks(ui_state_t *state,
                                          navigation_callback_t nav_callback,
                                          void *user_data);

// Управление состоянием
void ui_controller_update_state(ui_state_t *state);
void ui_controller_render_current_screen(ui_state_t *state);

// Интеграция с AppContext
int ui_controller_sync_with_app_context(ui_state_t *state);

// Действия
void ui_controller_action_quit(ui_state_t *state, void *user_data);
void ui_controller_action_help(ui_state_t *state, void *user_data);
void ui_controller_action_refresh(ui_state_t *state, void *user_data);
void ui_controller_action_toggle_colors(ui_state_t *state, void *user_data);
void ui_controller_action_continue(ui_state_t *state, void *user_data);
void ui_controller_action_toggle_theme(ui_state_t *state, void *user_data);
void ui_controller_action_settings(ui_state_t *state, void *user_data);
void ui_controller_action_diagnostics(ui_state_t *state, void *user_data);

// Прямые переключения экранов
void ui_controller_switch_to_main(ui_state_t *state, void *user_data);
void ui_controller_switch_to_settings(ui_state_t *state, void *user_data);
void ui_controller_switch_to_diagnostics(ui_state_t *state, void *user_data);
void ui_controller_switch_to_developer(ui_state_t *state, void *user_data);

// Экран тестирования системы
int ui_controller_run_system_test(ui_state_t *state);
void ui_controller_show_testing_screen(ui_state_t *state, float progress);

// Экран настроек
typedef struct {
    app_settings_t settings;          // Текущие настройки
    int current_item;                 // Выбранный элемент меню
    int needs_save;                   // Нужно ли сохранять изменения
    char config_file_path[256];       // Путь к файлу конфигурации
} settings_screen_state_t;

// Глобальное состояние экрана настроек
extern settings_screen_state_t g_settings_state;

// Инициализация экрана настроек
int ui_controller_init_settings_screen(ui_state_t *state, const char *config_file);

// Обработка ввода в экране настроек
int ui_controller_handle_settings_input(ui_state_t *state, int ch);

// Отрисовка экрана настроек
void ui_controller_show_settings_screen(ui_state_t *state);

// Сохранение настроек
int ui_controller_save_settings(ui_state_t *state);

// Загрузка настроек
int ui_controller_load_settings(ui_state_t *state);

// Сброс настроек к значениям по умолчанию
void ui_controller_reset_settings_to_defaults(ui_state_t *state);

// Действия для экрана настроек
void ui_controller_settings_action_save(ui_state_t *state, void *user_data);
void ui_controller_settings_action_cancel(ui_state_t *state, void *user_data);
void ui_controller_settings_action_reset(ui_state_t *state, void *user_data);
void ui_controller_settings_action_navigate_up(ui_state_t *state, void *user_data);
void ui_controller_settings_action_navigate_down(ui_state_t *state, void *user_data);
void ui_controller_settings_action_value_up(ui_state_t *state, void *user_data);
void ui_controller_settings_action_value_down(ui_state_t *state, void *user_data);

#endif // UI_CONTROLLER_H