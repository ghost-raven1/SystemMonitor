/**
 * UI State Manager Module
 *
 * Централизованное управление состоянием интерфейса.
 * Интегрируется с AppContext для глобального состояния.
 */

#ifndef UI_STATE_MANAGER_H
#define UI_STATE_MANAGER_H

#include "ui_data_visualizer.h"
#include "ui_input_handler.h"
#include "../core/app_context.h"

// Структура определена в ui_renderer.h для избежания дублирования
// Не используем typedef здесь, чтобы избежать конфликтов

// Функции инициализации
ui_state_t *ui_state_manager_init(app_context_t *ctx, int colors_enabled);
void ui_state_manager_cleanup(ui_state_t *state);

// Управление состоянием
void ui_state_set_current_screen(ui_state_t *state, screen_type_t screen);
screen_type_t ui_state_get_current_screen(ui_state_t *state);
void ui_state_set_needs_redraw(ui_state_t *state, int needs_redraw);
int ui_state_needs_redraw(ui_state_t *state);

// Обновление данных
void ui_state_update_system_metrics(ui_state_t *state);
void ui_state_update_metrics_history(ui_state_t *state, float cpu, float memory);

// Интеграция с AppContext
int ui_state_sync_with_app_context(ui_state_t *state);
void ui_state_notify_app_context(ui_state_t *state, const char *event);

#endif // UI_STATE_MANAGER_H