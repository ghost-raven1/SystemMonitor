/**
 * UI Input Handler Module Implementation
 *
 * Реализация функций обработки пользовательского ввода и навигации.
 */

#include "ui/ui_input_handler.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Определения screen_type_t для использования в этом файле
typedef enum {
    SCREEN_MAIN,
    SCREEN_PROCESSES,
    SCREEN_PROCESS_TREE,
    SCREEN_NETWORK,
    SCREEN_DIAGNOSTICS,
    SCREEN_TESTING,
    SCREEN_SETTINGS,
    SCREEN_WEATHER,
    SCREEN_DOCKER,
    SCREEN_GIT_REPOS,
    SCREEN_DEV_ENV,
    SCREEN_LISTENING_PORTS
} screen_type_t;

static int g_timeout_ms = 2000; // Таймаут по умолчанию 2 секунды

void ui_input_init(void) {
    // Инициализация модуля ввода
    // Здесь можно добавить специфичные настройки ncurses для ввода
}

void ui_input_cleanup(void) {
    // Очистка ресурсов модуля ввода
}

input_state_t *ui_input_create_state(input_mode_t mode, int total_items, int items_per_page) {
    input_state_t *state = calloc(1, sizeof(input_state_t));
    if (!state) return NULL;

    state->mode = mode;
    state->total_items = total_items;
    state->items_per_page = items_per_page;
    state->timeout_ms = g_timeout_ms;
    state->filter_text[0] = '\0';

    return state;
}

void ui_input_destroy_state(input_state_t *state) {
    if (state) {
        free(state);
    }
}

int ui_handle_navigation_input(input_state_t *state, int ch,
                              navigation_callback_t nav_callback, void *user_data) {
    if (!state) return 0;

    switch (ch) {
        case KEY_UP:
        case 'w':
        case 'W':
            if (state->selected_item > 0) {
                state->selected_item--;
                if (state->selected_item < state->page_offset) {
                    state->page_offset = state->selected_item;
                }
                if (nav_callback) nav_callback(-1, user_data); // Вверх
            }
            break;

        case KEY_DOWN:
        case 's':
        case 'S':
            if (state->selected_item < state->total_items - 1) {
                state->selected_item++;
                if (state->selected_item >= state->page_offset + state->items_per_page) {
                    state->page_offset = state->selected_item - state->items_per_page + 1;
                }
                if (nav_callback) nav_callback(1, user_data); // Вниз
            }
            break;

        case UI_KEY_PGUP:
            state->page_offset -= state->items_per_page;
            if (state->page_offset < 0) state->page_offset = 0;
            state->selected_item = state->page_offset;
            if (nav_callback) nav_callback(-2, user_data); // Страница вверх
            break;

        case UI_KEY_PGDN:
            state->page_offset += state->items_per_page;
            if (state->page_offset >= state->total_items) {
                state->page_offset = state->total_items - state->items_per_page;
            }
            if (state->page_offset < 0) state->page_offset = 0;
            state->selected_item = state->page_offset;
            if (nav_callback) nav_callback(2, user_data); // Страница вниз
            break;

        case UI_KEY_QUIT:
        case UI_KEY_ESCAPE:
            return 0; // Выйти из цикла обработки

        default:
            return 1; // Продолжить обработку
    }

    return 1; // Продолжить обработку
}

int ui_handle_filter_input(input_state_t *state, int ch,
                          filter_callback_t filter_callback, void *user_data) {
    if (!state) return 0;

    if (ch == '/') {
        // Начать ввод фильтра
        state->mode = INPUT_MODE_FILTER;

        // Очистить экран и показать промпт
        // Это будет реализовано в основном UI модуле

        if (filter_callback) {
            filter_callback(state->filter_text, user_data);
        }

        return 1;
    }

    return ch; // Вернуть клавишу для дальнейшей обработки
}

int ui_handle_confirmation_input(input_state_t *state, int ch, const char *prompt) {
    if (!state || !prompt) return -1;

    state->mode = INPUT_MODE_CONFIRM;

    // Здесь можно показать диалог подтверждения
    // Это будет реализовано в основном UI модуле

    if (ch == 'y' || ch == 'Y') {
        return 1; // Подтверждено
    } else if (ch == 'n' || ch == 'N' || ch == UI_KEY_ESCAPE) {
        return 0; // Отменено
    }

    return -1; // Продолжить ожидание
}

int ui_handle_text_input(char *buffer, size_t buffer_size, int ch) {
    if (!buffer || buffer_size == 0) return -1;

    static size_t cursor = 0;

    if (ch == KEY_ENTER || ch == '\n' || ch == '\r') {
        return (int)cursor; // Вернуть длину текста
    }

    if (ch == KEY_BACKSPACE || ch == 127) {
        if (cursor > 0) {
            cursor--;
            buffer[cursor] = '\0';
        }
        return -1; // Продолжить ввод
    }

    if (ch == UI_KEY_ESCAPE) {
        buffer[0] = '\0';
        cursor = 0;
        return -1; // Отменено
    }

    if (isprint(ch) && cursor < buffer_size - 1) {
        buffer[cursor] = (char)ch;
        cursor++;
        buffer[cursor] = '\0';
    }

    return -1; // Продолжить ввод
}

void ui_input_set_selection(input_state_t *state, int new_selection) {
    if (!state) return;

    if (new_selection >= 0 && new_selection < state->total_items) {
        state->selected_item = new_selection;

        // Обновить смещение страницы при необходимости
        if (state->selected_item < state->page_offset) {
            state->page_offset = state->selected_item;
        } else if (state->selected_item >= state->page_offset + state->items_per_page) {
            state->page_offset = state->selected_item - state->items_per_page + 1;
        }

        if (state->page_offset < 0) state->page_offset = 0;
    }
}

int ui_input_get_selection(input_state_t *state) {
    return state ? state->selected_item : -1;
}

void ui_input_set_filter(input_state_t *state, const char *filter) {
    if (!state || !filter) return;

    strncpy(state->filter_text, filter, sizeof(state->filter_text) - 1);
    state->filter_text[sizeof(state->filter_text) - 1] = '\0';
}

const char *ui_input_get_filter(input_state_t *state) {
    return state ? state->filter_text : "";
}

int ui_is_navigation_key(int ch) {
    switch (ch) {
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
        case UI_KEY_PGUP:
        case UI_KEY_PGDN:
        case 'w':
        case 'W':
        case 's':
        case 'S':
        case 'a':
        case 'A':
        case 'd':
        case 'D':
            return 1;
        default:
            return 0;
    }
}

int ui_is_exit_key(int ch) {
    switch (ch) {
        case UI_KEY_QUIT:
        case 'Q':
        case UI_KEY_ESCAPE:
            return 1;
        default:
            return 0;
    }
}

int ui_is_help_key(int ch) {
    switch (ch) {
        case KEY_HELP:
        case '?':
            return 1;
        default:
            return 0;
    }
}

void ui_input_set_timeout(int timeout_ms) {
    g_timeout_ms = timeout_ms;
}

int ui_input_get_timeout(void) {
    return g_timeout_ms;
}

/**
 * @brief Основная функция обработки ввода для главного цикла
 * @param state - состояние ввода
 * @param nav_callback - коллбэк для навигации
 * @param user_data - пользовательские данные для коллбэка
 * @return 1 если продолжить обработку, 0 если выйти
 */
int ui_input_handler_process(input_state_t *state,
                            navigation_callback_t nav_callback,
                            void *user_data) {
    if (!state) return 1;

    // Получаем клавишу с таймаутом
    int ch = getch();

    if (ch == ERR) {
        // Таймаут или нет ввода
        return 1; // Продолжить обработку
    }

    // Обрабатываем ввод в зависимости от режима
    switch (state->mode) {
        case INPUT_MODE_NAVIGATION:
            return ui_handle_navigation_input(state, ch, nav_callback, user_data);

        case INPUT_MODE_FILTER:
            // Обработка фильтрации будет реализована в основном цикле
            return 1;

        case INPUT_MODE_CONFIRM:
            // Обработка подтверждения будет реализована в основном цикле
            return 1;

        case INPUT_MODE_TEXT:
            // Обработка текста будет реализована в основном цикле
            return 1;

        default:
            return 1;
    }
}

// Реализация новых функций для системы горячих клавиш

int ui_process_hotkey(input_state_t *state, int ch,
                      action_callback_t hotkey_callback, void *user_data) {
    if (!state || !hotkey_callback) return 0;

    switch (ch) {
        case 'r':
        case 265:  // KEY_F5
            hotkey_callback(user_data); // Обновить данные
            return 1;
        case 'q':
        case 27:   // KEY_ESCAPE
            hotkey_callback(user_data); // Выход
            return 1;
        case 'h':
        case 259:  // KEY_F1
            hotkey_callback(user_data); // Показать помощь
            return 1;
        case 't':
            hotkey_callback(user_data); // Переключить тему
            return 1;
        case 's':
            hotkey_callback(user_data); // Открыть настройки
            return 1;
        case 'd':
            hotkey_callback(user_data); // Открыть диагностику
            return 1;
        default:
            return 0;
    }
}

void ui_show_help_screen(input_state_t *state, void *user_data) {
    if (!state) return;

    // Здесь можно реализовать отображение экрана помощи
    // Для простоты просто печатаем в консоль
    printf("=== Справка по горячим клавишам ===\n");
    printf("Навигация:\n");
    printf("  Tab - Следующий экран\n");
    printf("  Shift+Tab - Предыдущий экран\n");
    printf("  1,2,3,4 - Прямой переход к экранам (1-основной, 2-настройки, 3-диагностика, 4-разработчик)\n");
    printf("Действия:\n");
    printf("  R, F5 - Обновить данные\n");
    printf("  Q, ESC - Выход из программы\n");
    printf("  H, F1 - Показать помощь\n");
    printf("  T - Переключить тему\n");
    printf("  S - Открыть настройки\n");
    printf("  D - Открыть диагностику\n");
    printf("Интерфейс:\n");
    printf("  Enter, Space - Подтверждение/продолжить\n");
    printf("  Стрелки - Навигация в списках\n");
    printf("  Page Up/Down - Прокрутка\n");
    printf("Нажмите любую клавишу для продолжения...\n");

    // Ждем ввода пользователя
    getch();
}

int ui_handle_navigation(input_state_t *state, int ch,
                         navigation_callback_t navigation_callback, void *user_data) {
    if (!state) return 0;

    // Обработка навигационных клавиш
    switch (ch) {
        case KEY_UP:
        case 'w':
        case 'W':
        case KEY_DOWN:
        case 's':
        case 'S':
        case KEY_LEFT:
        case 'a':
        case 'A':
        case KEY_RIGHT:
        case 'd':
        case 'D':
        case 339:  // KEY_PGUP
        case 338:  // KEY_PGDN
            return ui_handle_navigation_input(state, ch, navigation_callback, user_data);
        case '\n':  // Enter
        case ' ':   // Space
            if (navigation_callback) navigation_callback(0, user_data); // Подтверждение
            return 1;
        default:
            return 0;
    }
}

int ui_handle_screen_navigation(input_state_t *state, int ch,
                                void (*switch_screen_callback)(int screen, void *user_data),
                                void *user_data) {
    if (!state || !switch_screen_callback) return 0;

    screen_type_t target_screen = SCREEN_MAIN; // По умолчанию

    switch (ch) {
        case 9:    // KEY_TAB
            // Следующий экран - логика в контроллере
            return 0; // Не обрабатываем здесь, передаем в контроллер
        case 353:  // KEY_BTAB
            // Предыдущий экран
            return 0;
        case '1':
            target_screen = SCREEN_MAIN;
            break;
        case '2':
            target_screen = SCREEN_SETTINGS;
            break;
        case '3':
            target_screen = SCREEN_DIAGNOSTICS;
            break;
        case '4':
            target_screen = SCREEN_TESTING; // Разработчик - тестирование
            break;
        default:
            return 0;
    }

    switch_screen_callback(target_screen, user_data);
    return 1;
}