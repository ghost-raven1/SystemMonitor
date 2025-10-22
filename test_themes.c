#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "src/ui/ui_theme.h"

int main() {
    // Инициализируем ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // Инициализируем систему тем
    if (ui_theme_init() != 0) {
        endwin();
        printf("Ошибка инициализации системы тем\n");
        return 1;
    }

    // Тестируем переключение тем
    theme_type_t themes[] = {THEME_BTOP_DARK, THEME_NEON, THEME_MATRIX, THEME_LIGHT};
    const char* theme_names[] = {"BTOP_DARK", "NEON", "MATRIX", "LIGHT"};

    int current_theme = 0;

    while (1) {
        // Очищаем экран
        clear();

        // Показываем текущую тему
        mvprintw(2, 2, "Текущая тема: %s", theme_names[current_theme]);
        mvprintw(3, 2, "Нажмите 'n' для следующей темы, 'q' для выхода");

        // Применяем тему ко всему экрану
        ui_theme_apply_to_screen();

        // Показываем пример элементов интерфейса с темами
        int y = 6;

        // Заголовок
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
        mvprintw(y, 2, "ЗАГОЛОВОК В СТИЛЕ ТЕМЫ");
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TITLE]));
        y += 2;

        // Текст
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
        mvprintw(y, 2, "Это обычный текст с примененной темой");
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT]));
        y += 2;

        // Приглушенный текст
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
        mvprintw(y, 2, "Это приглушенный текст");
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_TEXT_DIM]));
        y += 2;

        // Рамка
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
        for (int i = 0; i < 30; i++) {
            mvaddch(y, 2 + i, symbols.border_h[0]);
            mvaddch(y + 4, 2 + i, symbols.border_h[0]);
        }
        mvaddch(y, 2, symbols.corner_tl[0]);
        mvaddch(y, 31, symbols.corner_tr[0]);
        mvaddch(y + 4, 2, symbols.corner_bl[0]);
        mvaddch(y + 4, 31, symbols.corner_br[0]);

        for (int i = 1; i < 4; i++) {
            mvaddch(y + i, 2, symbols.border_v[0]);
            mvaddch(y + i, 31, symbols.border_v[0]);
        }
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_BORDER]));
        y += 6;

        // Успешное сообщение
        attron(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_SUCCESS]));
        mvprintw(y, 2, "✓ Тема успешно применена!");
        attroff(COLOR_PAIR(current_theme.color_pairs[COLOR_BTOP_SUCCESS]));

        // Обновляем экран
        refresh();

        // Ожидаем ввода
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'n' || ch == 'N') {
            current_theme = (current_theme + 1) % 4;

            // Переключаем тему
            ui_theme_set_theme(themes[current_theme]);

            // Небольшая пауза для визуального эффекта
            usleep(200000);
        }
    }

    // Завершаем работу
    endwin();
    printf("Тестирование тем завершено успешно!\n");

    // Показываем информацию о текущей теме
    printf("Последняя активная тема: %s\n", ui_theme_get_current_theme_name());

    return 0;
}