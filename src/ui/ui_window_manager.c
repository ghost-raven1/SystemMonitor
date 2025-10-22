/**
 * UI Window Manager Module Implementation
 *
 * Реализация координации между модулями UI.
 */

#include "ui/ui_window_manager.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Глобальные коллбэки
static screen_change_callback_t g_screen_change_cb = NULL;
static data_update_callback_t g_data_update_cb = NULL;
static void *g_user_data = NULL;

// Обработчик сигнала изменения размера терминала
static volatile sig_atomic_t g_terminal_resized = 0;
static void handle_sigwinch(int sig) {
    (void)sig;
    g_terminal_resized = 1;
}

ui_state_t *ui_window_manager_init(int colors_enabled) {
    ui_state_t *state = calloc(1, sizeof(ui_state_t));
    if (!state) return NULL;

    // Инициализация модулей
    ui_renderer_init();
    ui_input_init();
    ui_data_visualizer_init();

    state->colors_enabled = colors_enabled;
    state->current_screen = SCREEN_MAIN;
    state->running = 1;
    state->needs_redraw = 1;

    // Создание состояния ввода для главного экрана
    state->input_state = ui_input_create_state(INPUT_MODE_NAVIGATION, 100, 15);
    if (!state->input_state) {
        free(state);
        return NULL;
    }

    // Инициализация истории метрик
    memset(&state->metrics_history, 0, sizeof(metrics_history_t));

    // Установка обработчика сигнала изменения размера
    signal(SIGWINCH, handle_sigwinch);

    return state;
}

void ui_window_manager_cleanup(ui_state_t *state) {
    if (!state) return;

    if (state->input_state) {
        ui_input_destroy_state(state->input_state);
    }

    ui_data_visualizer_cleanup();
    ui_input_cleanup();
    ui_renderer_cleanup();

    free(state);
}

void ui_run_main_loop(ui_state_t *state,
                     screen_change_callback_t screen_cb,
                     data_update_callback_t data_cb,
                     void *user_data) {
    if (!state) return;

    g_screen_change_cb = screen_cb;
    g_data_update_cb = data_cb;
    g_user_data = user_data;

    while (state->running) {
        // Обработка изменения размера терминала
        if (g_terminal_resized) {
            ui_handle_terminal_resize(state);
            g_terminal_resized = 0;
        }

        // Обновление данных
        ui_update_system_metrics(state);

        // Обработка ввода
        int ch = getch();
        if (ch != ERR) {
            if (!ui_handle_user_input(state, ch)) {
                break; // Выход из цикла
            }
        }

        // Отрисовка интерфейса
        ui_render_current_screen(state);

        // Вызов коллбэка обновления данных
        if (g_data_update_cb) {
            g_data_update_cb(&state->system_metrics, g_user_data);
        }

        // Небольшая пауза для предотвращения чрезмерной загрузки CPU
        napms(100);
    }
}

void ui_switch_to_screen(ui_state_t *state, screen_type_t screen) {
    if (!state) return;

    if (state->current_screen != screen) {
        state->current_screen = screen;
        state->needs_redraw = 1;

        // Обновление состояния ввода для нового экрана
        if (state->input_state) {
            ui_input_destroy_state(state->input_state);
        }

        // Создание нового состояния ввода в зависимости от экрана
        switch (screen) {
            case SCREEN_PROCESSES:
                state->input_state = ui_input_create_state(INPUT_MODE_NAVIGATION, 256, 15);
                break;
            case SCREEN_PROCESS_TREE:
                state->input_state = ui_input_create_state(INPUT_MODE_NAVIGATION, 50, 10);
                break;
            default:
                state->input_state = ui_input_create_state(INPUT_MODE_NAVIGATION, 100, 15);
                break;
        }

        // Вызов коллбэка смены экрана
        if (g_screen_change_cb) {
            g_screen_change_cb(screen, g_user_data);
        }
    }
}

void ui_update_system_metrics(ui_state_t *state) {
    if (!state) return;

    ui_get_system_metrics(&state->system_metrics);
    ui_update_metrics_history(&state->metrics_history,
                             state->system_metrics.cpu_usage,
                             state->system_metrics.memory_usage);
}

int ui_handle_user_input(ui_state_t *state, int ch) {
    if (!state || !state->input_state) return 1;

    // Обработка глобальных команд
    switch (ch) {
        case 'q':
        case 'Q':
        case 27: // ESC
            state->running = 0;
            return 0;

        case 'p':
        case 'P':
            ui_switch_to_screen(state, SCREEN_PROCESSES);
            return 1;

        case 't':
        case 'T':
            ui_switch_to_screen(state, SCREEN_PROCESS_TREE);
            return 1;

        case 'n':
        case 'N':
            ui_switch_to_screen(state, SCREEN_NETWORK);
            return 1;

        case 'd':
        case 'D':
            ui_switch_to_screen(state, SCREEN_DIAGNOSTICS);
            return 1;

        case 'w':
        case 'W':
            ui_switch_to_screen(state, SCREEN_WEATHER);
            return 1;

        case 'c':
        case 'C':
            ui_switch_to_screen(state, SCREEN_DOCKER);
            return 1;

        case 'g':
        case 'G':
            ui_switch_to_screen(state, SCREEN_GIT_REPOS);
            return 1;

        case 'e':
        case 'E':
            ui_switch_to_screen(state, SCREEN_DEV_ENV);
            return 1;

        case 'l':
        case 'L':
            ui_switch_to_screen(state, SCREEN_LISTENING_PORTS);
            return 1;

        case 's':
        case 'S':
            // Сохранение снимка системы
            ui_save_snapshot(state, "system_snapshot.txt");
            return 1;

        default:
            // Обработка навигационных команд текущего экрана
            return ui_handle_navigation_input(state->input_state, ch, NULL, NULL);
    }

    return 1;
}

void ui_render_current_screen(ui_state_t *state) {
    if (!state) return;

    clear();

    switch (state->current_screen) {
        case SCREEN_MAIN:
            ui_show_main_screen(&state->system_metrics,
                              &state->metrics_history,
                              state->colors_enabled);
            break;

        case SCREEN_PROCESSES:
            ui_show_processes_screen(state->input_state, state->colors_enabled);
            break;

        case SCREEN_PROCESS_TREE:
            ui_show_process_tree_screen(state->colors_enabled);
            break;

        case SCREEN_NETWORK:
            ui_show_network_screen(state->colors_enabled);
            break;

        case SCREEN_DIAGNOSTICS:
            ui_show_diagnostics_screen(state->colors_enabled);
            break;

        case SCREEN_WEATHER:
            ui_show_weather_details_screen(state->colors_enabled);
            break;

        case SCREEN_DOCKER:
            ui_show_docker_screen(state->colors_enabled);
            break;

        case SCREEN_GIT_REPOS:
            ui_show_git_repos_screen(state->colors_enabled);
            break;

        case SCREEN_DEV_ENV:
            ui_show_dev_environment_screen(state->colors_enabled);
            break;

        case SCREEN_LISTENING_PORTS:
            ui_show_listening_ports_screen(state->colors_enabled);
            break;
    }

    state->needs_redraw = 0;
    refresh();
}

void ui_set_screen_change_callback(ui_state_t *state,
                                  screen_change_callback_t callback,
                                  void *user_data) {
    g_screen_change_cb = callback;
    g_user_data = user_data;
}

void ui_set_data_update_callback(ui_state_t *state,
                                data_update_callback_t callback,
                                void *user_data) {
    g_data_update_cb = callback;
    g_user_data = user_data;
}

screen_type_t ui_get_current_screen(ui_state_t *state) {
    return state ? state->current_screen : SCREEN_MAIN;
}

int ui_needs_redraw(ui_state_t *state) {
    return state ? state->needs_redraw : 1;
}

void ui_set_needs_redraw(ui_state_t *state, int needs_redraw) {
    if (state) {
        state->needs_redraw = needs_redraw;
    }
}

input_state_t *ui_get_input_state(ui_state_t *state) {
    return state ? state->input_state : NULL;
}

system_metrics_t *ui_get_system_metrics_from_window_manager(ui_state_t *state) {
    return state ? &state->system_metrics : NULL;
}

int ui_save_snapshot(ui_state_t *state, const char *filename) {
    if (!state || !filename) return -1;

    return ui_save_system_snapshot(filename, &state->system_metrics);
}

void ui_handle_terminal_resize(ui_state_t *state) {
    if (!state) return;

    endwin();
    refresh();
    clear();
    state->needs_redraw = 1;
}

int ui_check_module_availability(ui_state_t *state) {
    if (!state) return 0;

    int available = 0;

    // Проверка доступности различных модулей
    // Здесь можно добавить реальную логику проверки

    return available;
}

/**
 * @brief Основная функция рендеринга для главного цикла
 * @param state Состояние UI
 * @return 0 при успехе, -1 при ошибке
 */
int ui_window_manager_render(ui_state_t *state) {
    if (!state) return -1;

    // Проверяем необходимость перерисовки
    if (!ui_needs_redraw(state)) {
        return 0;
    }

    // Отрисовываем текущий экран
    ui_render_current_screen(state);

    return 0;
}