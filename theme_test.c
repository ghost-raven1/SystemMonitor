#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "src/ui/ui_theme.h"
#include "src/utils/logging.h"

int main() {
    // Инициализируем ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Инициализируем цвета если поддерживаются
    if (has_colors()) {
        start_color();
    }

    // Инициализируем систему тем
    if (ui_theme_init() != 0) {
        printf("Ошибка инициализации системы тем\n");
        endwin();
        return 1;
    }

    // Тестируем различные темы
    theme_type_t themes[] = {THEME_BTOP_DARK, THEME_NEON, THEME_MATRIX, THEME_LIGHT};
    const char* theme_names[] = {"Темная", "Неоновая", "Матричная", "Светлая"};

    clear();
    mvprintw(2, 2, "Тестирование системы тем:");
    mvprintw(3, 2, "Нажмите любую клавишу для переключения темы");
    mvprintw(4, 2, "Q - выход");

    int current_theme = 0;

    while (1) {
        // Устанавливаем текущую тему
        ui_theme_set_theme(themes[current_theme]);

        // Очищаем экран
        clear();

        // Рисуем рамку
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        // Заголовок
        mvprintw(1, 2, "╔══════════════════════════════════════════════════════════════╗");
        mvprintw(2, 2, "║                    Тема: %s", theme_names[current_theme]);
        mvprintw(2, cols - 20, "║");
        mvprintw(3, 2, "╚══════════════════════════════════════════════════════════════╝");

        // Тестовые элементы
        mvprintw(5, 4, "Текст заголовка:");
        mvprintw(6, 4, "Обычный текст");
        mvprintw(7, 4, "Цветовые акценты:");

        // Цветовые тесты
        if (has_colors()) {
            mvprintw(8, 4, "Цвет успеха");
            mvprintw(9, 4, "Цвет предупреждения");
            mvprintw(10, 4, "Цвет ошибки");
            mvprintw(11, 4, "Цвет информации");
        }

        // Градиентный бар
        mvprintw(13, 4, "Градиентный бар (75%%):");
        mvprintw(15, 4, "Нажмите клавишу для смены темы, Q для выхода");

        refresh();

        // Ожидаем ввод
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        current_theme = (current_theme + 1) % 4;
    }

    // Деинициализируем ncurses
    endwin();

    printf("Тестирование системы тем завершено успешно!\n");
    return 0;
}