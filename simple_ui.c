#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

int main() {
    printf("Запуск простого ncurses теста...\n");

    // Enable UTF-8 for ncurses drawing to avoid garbled characters
    setlocale(LC_ALL, "");

    // Инициализируем ncurses
    if (initscr() == NULL) {
        fprintf(stderr, "Error: Unable to initialize ncurses terminal\n");
        return 1;
    }

    printf("ncurses инициализирован успешно\n");

    // Настройки для корректной работы
    cbreak();        // Включаем cbreak режим для немедленного ввода
    noecho();        // Отключаем echo
    curs_set(FALSE); // Скрываем курсор

    int colors_on = has_colors();
    printf("Цвета поддерживаются: %d\n", colors_on);

    if (colors_on) {
        if (start_color() == ERR) {
            fprintf(stderr, "Warning: Unable to initialize colors\n");
            colors_on = 0;
        } else {
            printf("Цвета инициализированы успешно\n");
        }
    }

    // Настройки для максимальной совместимости
    keypad(stdscr, TRUE);  // Включаем keypad для поддержки стрелок и функциональных клавиш
    nodelay(stdscr, FALSE); // Отключаем nodelay для корректной блокировки на ввод

    // prevent mouse/gesture input from confusing ncurses
    mousemask(0, NULL);

    // Принудительный сброс терминального буфера
    flushinp();

    if (colors_on) {
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    }

    // Простой интерфейс
    clear();
    mvprintw(5, 10, "╔════════════════════════════════╗");
    mvprintw(6, 10, "║    SystemMonitor v2.0          ║");
    mvprintw(7, 10, "║                                ║");
    mvprintw(8, 10, "║    Простой тестовый интерфейс  ║");
    mvprintw(9, 10, "║                                ║");
    mvprintw(10, 10, "║    Нажмите 'q' для выхода     ║");
    mvprintw(11, 10, "╚════════════════════════════════╝");

    refresh();

    // Основной цикл
    while (1) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        napms(100);
    }

    endwin();
    printf("Программа завершена успешно\n");
    return 0;
}