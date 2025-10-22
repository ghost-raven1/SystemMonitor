/**
 * UI Controller Implementation
 */

#include "ui_controller.h"
#include "ui_renderer.h"  // Для ui_state_t
#include "ui_data_visualizer.h"  // Для screen_type_t
#include "ui_theme.h"  // Для функций темы
#include "../utils/logging.h"
#include "../core/app_context.h"
// theme_type_t уже определен в ui_renderer.h
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static hotkey_t hotkeys[] = {
    {KEY_QUIT, (ui_action_callback_t)ui_controller_action_quit, "Выход"},
    {KEY_REFRESH, (ui_action_callback_t)ui_controller_action_refresh, "Обновить"},
    {KEY_HELP, (ui_action_callback_t)ui_controller_action_help, "Помощь"},
    {'c', (ui_action_callback_t)ui_controller_action_toggle_colors, "Переключить цвета"},
    {KEY_TAB, (ui_action_callback_t)ui_controller_next_screen, "Следующий экран"},
    {KEY_BTAB, (ui_action_callback_t)ui_controller_prev_screen, "Предыдущий экран"},
    {'\n', (ui_action_callback_t)ui_controller_action_continue, "Продолжить"}, // Enter
    {' ', (ui_action_callback_t)ui_controller_action_continue, "Продолжить"},  // Space
    {'t', (ui_action_callback_t)ui_controller_action_toggle_theme, "Переключить тему"},
    {'s', (ui_action_callback_t)ui_controller_action_settings, "Открыть настройки"},
    {'d', (ui_action_callback_t)ui_controller_action_diagnostics, "Открыть диагностику"},
    {'1', (ui_action_callback_t)ui_controller_switch_to_main, "Основной экран"},
    {'2', (ui_action_callback_t)ui_controller_switch_to_settings, "Настройки"},
    {'3', (ui_action_callback_t)ui_controller_switch_to_diagnostics, "Диагностика"},
    {'4', (ui_action_callback_t)ui_controller_switch_to_developer, "Разработчик"},
    {0, NULL, NULL} // Конец списка
};

int ui_controller_init(ui_state_t *state, app_context_t *ctx) {
    if (!state) return -1;

    // Регистрировать горячие клавиши
    ui_controller_register_hotkeys(state);

    printf("UI Controller initialized\n");
    return 0;
}

void ui_controller_cleanup(void) {
    printf("UI Controller cleaned up\n");
}

int ui_controller_switch_screen(ui_state_t *state, screen_type_t screen) {
    if (!state) return -1;

    ui_state_set_current_screen(state, screen);
    // Обновляем метрики при переключении экрана
    ui_state_update_system_metrics(state);
    refresh();  // Обновляем экран сразу после переключения
    return 0;
}

int ui_controller_next_screen(ui_state_t *state) {
    if (!state) return -1;

    screen_type_t current = ui_state_get_current_screen(state);
    screen_type_t next = (current + 1) % (SCREEN_LISTENING_PORTS + 1);
    return ui_controller_switch_screen(state, next);
}

int ui_controller_prev_screen(ui_state_t *state) {
    if (!state) return -1;

    screen_type_t current = ui_state_get_current_screen(state);
    screen_type_t prev = (current - 1 + (SCREEN_LISTENING_PORTS + 1)) % (SCREEN_LISTENING_PORTS + 1);
    return ui_controller_switch_screen(state, prev);
}

int ui_controller_handle_input(ui_state_t *state, int ch) {
    if (!state) return -1;

    screen_type_t current_screen = ui_state_get_current_screen(state);

    // Обрабатываем ввод для экрана настроек отдельно
    if (current_screen == SCREEN_SETTINGS) {
        return ui_controller_handle_settings_input(state, ch);
    }

    // Проверить горячие клавиши
    if (ui_controller_process_hotkey(state, ch)) {
        return 1; // Обработано
    }

    // Обработать навигацию с помощью input handler
    navigation_callback_t nav_callback = NULL; // TODO: Установить правильный коллбэк
    return ui_handle_navigation_input(state->input_state, ch, nav_callback, state);
}

int ui_controller_process_hotkey(ui_state_t *state, int ch) {
    for (int i = 0; hotkeys[i].key != 0; i++) {
        if (hotkeys[i].key == ch) {
            hotkeys[i].callback(state, NULL);
            return 1;
        }
    }
    return 0;
}

int ui_controller_register_hotkeys(ui_state_t *state) {
    // Здесь можно зарегистрировать в input handler
    printf("Hotkeys registered\n");
    return 0;
}

void ui_controller_set_navigation_callbacks(ui_state_t *state,
                                          navigation_callback_t nav_callback,
                                          void *user_data) {
    // Установить коллбэки
}

void ui_controller_update_state(ui_state_t *state) {
    if (!state) return;

    ui_state_update_system_metrics(state);
    ui_state_sync_with_app_context(state);
}

void ui_controller_render_current_screen(ui_state_t *state) {
    if (!state) return;

    screen_type_t screen = ui_state_get_current_screen(state);
    int colors = state->colors_enabled;

    switch (screen) {
        case SCREEN_MAIN:
            // Используем новую функцию рендеринга главного экрана в стиле btop
            {
                int rows, cols;
                getmaxyx(stdscr, rows, cols);
                ui_render_main_screen(rows, cols, state);
            }
            break;
        case SCREEN_DIAGNOSTICS:
            ui_render_diagnostics_screen();
            break;
        case SCREEN_TESTING:
            ui_show_testing_screen(colors);
            break;
        case SCREEN_SETTINGS:
            ui_controller_show_settings_screen(state);
            break;
        default:
            // Используем новую функцию рендеринга главного экрана в стиле btop по умолчанию
            {
                int rows, cols;
                getmaxyx(stdscr, rows, cols);
                ui_render_main_screen(rows, cols, state);
            }
            break;
    }

    ui_state_set_needs_redraw(state, 0);
}

int ui_controller_sync_with_app_context(ui_state_t *state) {
    return ui_state_sync_with_app_context(state);
}

// Действия
void ui_controller_action_quit(ui_state_t *state, void *user_data) {
    state->running = 0;
    printf("UI: Quit action\n");
}

void ui_controller_action_continue(ui_state_t *state, void *user_data) {
    // Переход к основному экрану после тестирования
    ui_controller_switch_screen(state, SCREEN_MAIN);
    printf("UI: Continue to main screen\n");
}

void ui_controller_action_help(ui_state_t *state, void *user_data) {
    printf("UI: Help - Use Tab to navigate, q to quit\n");
}

void ui_controller_action_refresh(ui_state_t *state, void *user_data) {
    ui_state_set_needs_redraw(state, 1);
    printf("UI: Refresh\n");
}

void ui_controller_action_toggle_colors(ui_state_t *state, void *user_data) {
    state->colors_enabled = !state->colors_enabled;
    ui_state_set_needs_redraw(state, 1);
    printf("UI: Colors toggled\n");
}

int ui_controller_run_system_test(ui_state_t *state) {
    if (!state) return -1;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Показываем экран тестирования
    ui_controller_switch_screen(state, SCREEN_TESTING);

    // Симулируем процесс тестирования модулей
    float progress = 0.0f;
    const float progress_step = 100.0f / 6.0f; // 6 этапов тестирования

    // Этап 1: Проверка базовой системы
    progress += progress_step;
    ui_controller_show_testing_screen(state, progress);
    usleep(500000); // 0.5 секунды

    // Этап 2: Проверка system_monitor модуля
    progress += progress_step;
    ui_controller_show_testing_screen(state, progress);
    usleep(300000);

    // Этап 3: Проверка developer_tools модуля
    progress += progress_step;
    ui_controller_show_testing_screen(state, progress);
    usleep(300000);

    // Этап 4: Проверка diagnostics модуля
    progress += progress_step;
    ui_controller_show_testing_screen(state, progress);
    usleep(300000);

    // Этап 5: Проверка error_handler модуля
    progress += progress_step;
    ui_controller_show_testing_screen(state, progress);
    usleep(300000);

    // Этап 6: Финализация
    progress = 100.0f;
    ui_controller_show_testing_screen(state, progress);

    // После завершения тестирования переходим к главному экрану
    ui_controller_switch_screen(state, SCREEN_MAIN);

    printf("UI: System test completed\n");
    return 0;
}

void ui_controller_show_testing_screen(ui_state_t *state, float progress) {
    if (!state) return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Получаем контекст приложения для проверки модулей
    app_context_t *ctx = get_app_context();
    if (!ctx) return;

    // Создаем строку со статусом модулей
    char module_status[256] = "";

    // Проверяем каждый модуль
    if (is_module_initialized("system_monitor")) {
        strcat(module_status, "[✓] System Monitor ");
    } else {
        strcat(module_status, "[✗] System Monitor ");
    }

    if (is_module_initialized("developer_tools")) {
        strcat(module_status, "[✓] Developer Tools ");
    } else {
        strcat(module_status, "[!] Developer Tools ");
    }

    if (is_module_initialized("diagnostics")) {
        strcat(module_status, "[✓] Diagnostics ");
    } else {
        strcat(module_status, "[!] Diagnostics ");
    }

    if (is_module_initialized("error_handler")) {
        strcat(module_status, "[✓] Error Handler");
    } else {
        strcat(module_status, "[✗] Error Handler");
    }

    // Используем функцию рендерера для отрисовки экрана тестирования
    ui_render_testing_screen(rows, cols, progress, module_status, state->colors_enabled);

    // Обновляем экран
    refresh();

    // Обновляем экран
    refresh();
}

// Новые действия для горячих клавиш

void ui_controller_action_toggle_theme(ui_state_t *state, void *user_data) {
    if (!state) return;

    // Получаем настройки из глобального состояния экрана настроек
    app_settings_t *settings = &g_settings_state.settings;

    // Циклически переключаем темы
    theme_type_t current_theme = settings->theme;
    theme_type_t next_theme = (current_theme + 1) % (THEME_LIGHT + 1);

    // Применяем новую тему
    ui_theme_set_theme(next_theme);

    // Обновляем настройки
    settings->theme = next_theme;
    g_settings_state.needs_save = 1;

    // Синхронизируем с AppContext
    app_context_t *ctx = get_app_context();
    if (ctx) {
        // Здесь можно добавить синхронизацию темы с AppContext
    }

    // Обновляем состояние UI
    state->colors_enabled = settings->use_colors;
    ui_state_set_needs_redraw(state, 1);

    printf("UI: Theme switched to %d\n", next_theme);
}

void ui_controller_action_settings(ui_state_t *state, void *user_data) {
    ui_controller_switch_screen(state, SCREEN_SETTINGS);
    printf("UI: Switched to Settings\n");
}

void ui_controller_action_diagnostics(ui_state_t *state, void *user_data) {
    ui_controller_switch_screen(state, SCREEN_DIAGNOSTICS);
    printf("UI: Switched to Diagnostics\n");
}

void ui_controller_switch_to_main(ui_state_t *state, void *user_data) {
    ui_controller_switch_screen(state, SCREEN_MAIN);
    printf("UI: Switched to Main\n");
}

void ui_controller_switch_to_settings(ui_state_t *state, void *user_data) {
    ui_controller_switch_screen(state, SCREEN_SETTINGS);
    printf("UI: Switched to Settings\n");
}

void ui_controller_switch_to_diagnostics(ui_state_t *state, void *user_data) {
    ui_controller_switch_screen(state, SCREEN_DIAGNOSTICS);
    printf("UI: Switched to Diagnostics\n");
}

void ui_controller_switch_to_developer(ui_state_t *state, void *user_data) {
    // Для будущего экрана разработчика
    // Пока переключаемся на главный экран с сообщением
    ui_controller_switch_screen(state, SCREEN_MAIN);
    printf("UI: Developer screen - not implemented yet, switched to Main\n");
}

// Глобальное состояние экрана настроек
settings_screen_state_t g_settings_state;

// Инициализация экрана настроек
int ui_controller_init_settings_screen(ui_state_t *state, const char *config_file) {
    if (!state || !config_file) return -1;

    // Инициализируем настройки по умолчанию
    ui_settings_init_defaults(&g_settings_state.settings);

    // Загружаем настройки из файла
    if (ui_settings_load_config(&g_settings_state.settings, config_file) != 0) {
        log_error("Не удалось загрузить настройки, используются значения по умолчанию");
    }

    // Устанавливаем путь к файлу конфигурации
    strncpy(g_settings_state.config_file_path, config_file, sizeof(g_settings_state.config_file_path) - 1);
    g_settings_state.config_file_path[sizeof(g_settings_state.config_file_path) - 1] = '\0';

    // Сбрасываем состояние
    g_settings_state.current_item = 0;
    g_settings_state.needs_save = 0;

    log_info("Экран настроек инициализирован");
    return 0;
}

// Обработка ввода в экране настроек
int ui_controller_handle_settings_input(ui_state_t *state, int ch) {
    if (!state) return -1;

    int menu_count = ui_settings_get_menu_count();
    int handled = 0;

    switch (ch) {
        case KEY_UP:
        case 'k':
        case 'K':
            // Навигация вверх
            if (g_settings_state.current_item > 0) {
                g_settings_state.current_item--;
                g_settings_state.needs_save = 1;
                handled = 1;
            }
            break;

        case KEY_DOWN:
        case 'j':
        case 'J':
            // Навигация вниз
            if (g_settings_state.current_item < menu_count - 1) {
                g_settings_state.current_item++;
                g_settings_state.needs_save = 1;
                handled = 1;
            }
            break;

        case KEY_LEFT:
        case 'h':
        case 'H':
            // Уменьшить значение
            if (ui_settings_update_value(g_settings_state.current_item, 0, &g_settings_state.settings)) {
                g_settings_state.needs_save = 1;
                handled = 1;
            }
            break;

        case KEY_RIGHT:
        case 'l':
        case 'L':
            // Увеличить значение
            if (ui_settings_update_value(g_settings_state.current_item, 1, &g_settings_state.settings)) {
                g_settings_state.needs_save = 1;
                handled = 1;
            }
            break;

        case 's':
        case 'S':
            // Сохранить настройки
            ui_controller_settings_action_save(state, NULL);
            handled = 1;
            break;

        case 'c':
        case 'C':
            // Отмена (вернуться к предыдущему экрану)
            ui_controller_settings_action_cancel(state, NULL);
            handled = 1;
            break;

        case 'r':
        case 'R':
            // Сброс к значениям по умолчанию
            ui_controller_settings_action_reset(state, NULL);
            handled = 1;
            break;

        case 27: // ESC
            // Отмена
            ui_controller_settings_action_cancel(state, NULL);
            handled = 1;
            break;

        case KEY_ENTER:
        case '\n':
            // Переключить булево значение или перейти к следующему элементу
            if (ui_settings_update_value(g_settings_state.current_item, 2, &g_settings_state.settings)) {
                g_settings_state.needs_save = 1;
                handled = 1;
            }
            break;

        default:
            // Игнорируем другие клавиши
            break;
    }

    // Если ввод обработан, устанавливаем флаг перерисовки
    if (handled) {
        ui_state_set_needs_redraw(state, 1);
    }

    return handled;
}

// Отрисовка экрана настроек
void ui_controller_show_settings_screen(ui_state_t *state) {
    if (!state) return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Отрисовываем экран настроек
    ui_render_settings_screen(rows, cols, &g_settings_state.settings,
                             g_settings_state.current_item, state->colors_enabled);

    // Обновляем экран
    refresh();
}

// Сохранение настроек
int ui_controller_save_settings(ui_state_t *state) {
    if (!state) return -1;

    // Сохраняем настройки в файл
    if (ui_settings_save_config(&g_settings_state.settings, g_settings_state.config_file_path) == 0) {
        g_settings_state.needs_save = 0;

        // Применяем настройки к AppContext
        app_context_t *ctx = get_app_context();
        if (ctx) {
            // Обновляем интервал обновления в AppContext
            ctx->config.update_interval_ms = g_settings_state.settings.update_interval * 1000;

            // Обновляем другие настройки если нужно
            ctx->config.enable_networking = g_settings_state.settings.network_monitoring;
            ctx->config.enable_notifications = g_settings_state.settings.sound_notifications;

            // Синхронизируем тему с глобальным состоянием UI
            ui_theme_set_theme(g_settings_state.settings.theme);

            // Обновляем состояние UI
            if (state) {
                state->colors_enabled = g_settings_state.settings.use_colors;
                ui_state_set_needs_redraw(state, 1);
            }

            log_info("Настройки применены к AppContext");
        }

        log_info("Настройки успешно сохранены");

        // Возвращаемся к предыдущему экрану
        ui_controller_switch_screen(state, SCREEN_MAIN);
        return 0;
    } else {
        log_error("Не удалось сохранить настройки");
        return -1;
    }
}

// Загрузка настроек
int ui_controller_load_settings(ui_state_t *state) {
    if (!state) return -1;

    // Загружаем настройки из файла
    if (ui_settings_load_config(&g_settings_state.settings, g_settings_state.config_file_path) == 0) {
        g_settings_state.needs_save = 0;
        log_info("Настройки загружены");
        return 0;
    } else {
        log_error("Не удалось загрузить настройки");
        return -1;
    }
}

// Сброс настроек к значениям по умолчанию
void ui_controller_reset_settings_to_defaults(ui_state_t *state) {
    if (!state) return;

    // Сбрасываем к значениям по умолчанию
    ui_settings_init_defaults(&g_settings_state.settings);
    g_settings_state.needs_save = 1;

    log_info("Настройки сброшены к значениям по умолчанию");
}

// Действия для экрана настроек

void ui_controller_settings_action_save(ui_state_t *state, void *user_data) {
    ui_controller_save_settings(state);
}

void ui_controller_settings_action_cancel(ui_state_t *state, void *user_data) {
    // Если есть несохраненные изменения, спрашиваем подтверждение
    if (g_settings_state.needs_save) {
        // Пока просто выходим без сохранения
        log_info("Выход из настроек без сохранения изменений");
    }

    // Возвращаемся к предыдущему экрану
    ui_controller_switch_screen(state, SCREEN_MAIN);
}

void ui_controller_settings_action_reset(ui_state_t *state, void *user_data) {
    ui_controller_reset_settings_to_defaults(state);
}

void ui_controller_settings_action_navigate_up(ui_state_t *state, void *user_data) {
    if (g_settings_state.current_item > 0) {
        g_settings_state.current_item--;
        ui_state_set_needs_redraw(state, 1);
    }
}

void ui_controller_settings_action_navigate_down(ui_state_t *state, void *user_data) {
    int menu_count = ui_settings_get_menu_count();
    if (g_settings_state.current_item < menu_count - 1) {
        g_settings_state.current_item++;
        ui_state_set_needs_redraw(state, 1);
    }
}

void ui_controller_settings_action_value_up(ui_state_t *state, void *user_data) {
    if (ui_settings_update_value(g_settings_state.current_item, 1, &g_settings_state.settings)) {
        g_settings_state.needs_save = 1;
        ui_state_set_needs_redraw(state, 1);
    }
}

void ui_controller_settings_action_value_down(ui_state_t *state, void *user_data) {
    if (ui_settings_update_value(g_settings_state.current_item, 0, &g_settings_state.settings)) {
        g_settings_state.needs_save = 1;
        ui_state_set_needs_redraw(state, 1);
    }
}