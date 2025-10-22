// ui.c - Главный файл пользовательского интерфейса SystemMonitor
#include "ui_controller.h"
#include "ui_renderer.h"
#include "ui_state_manager.h"
#include "ui_theme.h"
#include "ui_diagnostics.h"
#include "ui_settings.c"
#include "../utils/logging.h"
#include "../core/app_context.h"
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

// Глобальное состояние UI
static ui_state_t *g_ui_state = NULL;

// Инициализация пользовательского интерфейса
static int ui_init(void) {
    // Настройка локали для корректного отображения UTF-8
    setlocale(LC_ALL, "ru_RU.UTF-8");
    setlocale(LC_CTYPE, "ru_RU.UTF-8");

    // Fallback на системную локаль если русская недоступна
    if (!setlocale(LC_ALL, "C.UTF-8")) {
        setlocale(LC_ALL, "");
    }

    // Инициализируем ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(1000); // 1 секунда таймаут

    // Включаем поддержку UTF-8 если доступна
    if (setlocale(LC_CTYPE, "ru_RU.UTF-8") || setlocale(LC_CTYPE, "C.UTF-8")) {
        // Терминал поддерживает UTF-8
        log_info("Включена поддержка UTF-8 для корректного отображения русских символов");
    } else {
        log_error("UTF-8 не поддерживается, используются ASCII символы");
    }

    // Инициализируем цвета если терминал поддерживает
    if (has_colors()) {
        start_color();
        ui_theme_init();
    }

    // Получаем контекст приложения
    g_app_context = get_app_context();
    if (!g_app_context) {
        log_error("Не удалось получить контекст приложения");
        return -1;
    }

    // Инициализируем состояние UI
    g_ui_state = ui_state_manager_init(g_app_context, has_colors() ? 1 : 0);
    if (!g_ui_state) {
        log_error("Не удалось инициализировать состояние UI");
        return -1;
    }

    // Инициализируем контроллер UI
    if (ui_controller_init(g_ui_state, g_app_context) != 0) {
        log_error("Не удалось инициализировать контроллер UI");
        return -1;
    }

    // Инициализируем экран настроек
    if (ui_controller_init_settings_screen(g_ui_state, "src/config.ini") != 0) {
        log_error("Не удалось инициализировать экран настроек");
        // Продолжаем работу даже если настройки не загрузились
    }

    log_info("UI инициализирован успешно");
    return 0;
}

// Деинициализация пользовательского интерфейса
static void ui_cleanup(void) {
    if (g_ui_state) {
        ui_state_manager_cleanup(g_ui_state);
        g_ui_state = NULL;
    }

    ui_controller_cleanup();

    endwin();
    log_info("UI деинициализирован");
}

// Основной цикл пользовательского интерфейса
static void ui_main_loop(void) {
    int ch;

    while (g_ui_state && g_ui_state->running) {
        // Обновляем состояние
        ui_controller_update_state(g_ui_state);

        // Отрисовываем текущий экран
        ui_controller_render_current_screen(g_ui_state);

        // Обрабатываем ввод
        ch = getch();
        if (ch != ERR) {
            ui_controller_handle_input(g_ui_state, ch);
        }

        // Небольшая пауза для предотвращения чрезмерной загрузки CPU
        usleep(50000); // 50ms
    }
}

// Функция запуска пользовательского интерфейса
void run_ui(void) {
    log_info("Запуск пользовательского интерфейса");

    // Инициализируем UI
    if (ui_init() != 0) {
        log_error("Не удалось инициализировать UI");
        return;
    }

    // Показываем экран тестирования системы при запуске
    if (ui_controller_run_system_test(g_ui_state) != 0) {
        log_error("Не удалось выполнить тестирование системы");
    }

    // Основной цикл UI
    ui_main_loop();

    // Деинициализируем UI
    ui_cleanup();

    log_info("Пользовательский интерфейс завершен");
}