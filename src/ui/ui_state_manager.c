/**
 * UI State Manager Implementation
 */

#include "ui_state_manager.h"
#include "ui_renderer.h"
#include "../utils/logging.h"
#include <stdlib.h>
#include <string.h>

ui_state_t *ui_state_manager_init(app_context_t *ctx, int colors_enabled) {
    ui_state_t *state = malloc(sizeof(ui_state_t));
    if (!state) {
        printf("UI Error: Failed to allocate UI state\n");
        return NULL;
    }

    state->current_screen = SCREEN_MAIN;
    state->input_state = ui_input_create_state(INPUT_MODE_NAVIGATION, 100, 20);
    state->colors_enabled = colors_enabled;
    state->running = 1;
    state->needs_redraw = 1;
    state->app_context = ctx;

    // Инициализировать метрики
    memset(&state->system_metrics, 0, sizeof(system_metrics_t));
    memset(&state->metrics_history, 0, sizeof(metrics_history_t));

    LOG_INFO("UI State Manager initialized", "ui_state_manager");
    return state;
}

void ui_state_manager_cleanup(ui_state_t *state) {
    if (!state) return;

    if (state->input_state) {
        ui_input_destroy_state(state->input_state);
    }

    free(state);
    LOG_INFO("UI State Manager cleaned up", "ui_state_manager");
}

void ui_state_set_current_screen(ui_state_t *state, screen_type_t screen) {
    if (!state) return;

    state->current_screen = screen;
    state->needs_redraw = 1;
    printf("UI: Switched to screen: %d\n", screen);
}

screen_type_t ui_state_get_current_screen(ui_state_t *state) {
    return state ? state->current_screen : SCREEN_MAIN;
}

void ui_state_set_needs_redraw(ui_state_t *state, int needs_redraw) {
    if (state) {
        state->needs_redraw = needs_redraw;
    }
}

int ui_state_needs_redraw(ui_state_t *state) {
    return state ? state->needs_redraw : 0;
}

void ui_state_update_system_metrics(ui_state_t *state) {
    if (!state) return;

    ui_get_system_metrics(&state->system_metrics);
    state->needs_redraw = 1;
}

void ui_state_update_metrics_history(ui_state_t *state, float cpu, float memory) {
    if (!state) return;

    ui_update_metrics_history(&state->metrics_history, cpu, memory);
    state->needs_redraw = 1;
}

int ui_state_sync_with_app_context(ui_state_t *state) {
    if (!state || !state->app_context) return -1;

    WITH_CONTEXT_LOCK({
        state->app_context->global_state.status = IS_APP_RUNNING() ? APP_STATUS_RUNNING : APP_STATUS_STOPPED;
        // Синхронизировать другие поля при необходимости
    });

    return 0;
}

void ui_state_notify_app_context(ui_state_t *state, const char *event) {
    if (!state || !state->app_context || !event) return;

    // Уведомить AppContext о событиях UI
    printf("UI: Event: %s\n", event);
}