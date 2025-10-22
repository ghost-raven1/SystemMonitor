/**
 * modern_ui_demo.c - Демонстрация нового дизайна интерфейса в стиле btop
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <math.h>
#include "ui/ui_theme.h"

void demo_interface_design() {
    printf("🎨 Демонстрация нового дизайна интерфейса в стиле btop\n");
    printf("════════════════════════════════════════════════════\n\n");

    // Инициализируем ncurses для демонстрации
    initscr();
    cbreak();
    noecho();
    curs_set(FALSE);

    if (has_colors()) {
        start_color();
        ui_theme_init();
        ui_theme_detect_terminal_colors();

        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        clear();

        // Демонстрируем различные элементы дизайна

        // 1. Заголовок в стиле btop
        ui_theme_set_theme(THEME_BTOP_DARK);
        ui_draw_modern_box(1, 2, 3, cols-4);
        mvprintw(2, cols/2-20, "%s СИСТЕМНЫЙ МОНИТОР v2.0 %s",
                ui_get_icon("cpu"), ui_get_icon("cpu"));

        // 2. Градиентные бары
        ui_draw_gradient_bar(5, 4, 30, 85, COLOR_BTOP_CPU_HIGH);
        mvprintw(5, 36, "CPU: 85%%");

        ui_draw_gradient_bar(7, 4, 30, 60, COLOR_BTOP_MEM_MED);
        mvprintw(7, 36, "MEM: 60%%");

        ui_draw_gradient_bar(9, 4, 30, 45, COLOR_BTOP_NET_LOW);
        mvprintw(9, 36, "NET: 45%%");

        // 3. Различные темы
        int demo_y = 12;
        ui_theme_set_theme(THEME_BTOP_NEON);
        mvprintw(demo_y++, 4, "⚡ Неоновая тема:");
        ui_draw_gradient_bar(demo_y++, 6, 25, 75, COLOR_BTOP_CPU_HIGH);

        ui_theme_set_theme(THEME_BTOP_MATRIX);
        mvprintw(demo_y++, 4, "🟢 Матричная тема:");
        ui_draw_gradient_bar(demo_y++, 6, 25, 75, COLOR_BTOP_CPU_HIGH);

        // 4. Спарклайн график
        float demo_values[20];
        for (int i = 0; i < 20; i++) {
            demo_values[i] = 20 + 60 * sin(i * 0.3);
        }
        mvprintw(18, 4, "Спарклайн график:");
        // ui_draw_sparkline(19, 4, demo_values, 20, COLOR_BTOP_ACCENT2);
        mvprintw(19, 6, "График нагрузки CPU (демонстрация значений)");

        // 5. Метрики с иконками
        int metrics_y = 21;
        ui_print_metric("Процессор:", "2.4 GHz", COLOR_BTOP_CPU_MED);
        mvprintw(metrics_y++, 4, "%s Загрузка: 45%%", ui_get_icon("cpu"));

        ui_print_metric("Память:", "8/16 GB", COLOR_BTOP_MEM_MED);
        mvprintw(metrics_y++, 4, "%s Использование: 60%%", ui_get_icon("mem"));

        ui_print_metric("Диск:", "/dev/sda1", COLOR_BTOP_NET_MED);
        mvprintw(metrics_y++, 4, "%s Свободно: 120 GB", ui_get_icon("disk"));

        ui_print_metric("Сеть:", "100 Mbps", COLOR_BTOP_NET_MED);
        mvprintw(metrics_y++, 4, "%s TX: 5.2 MB/s", ui_get_icon("net"));

        // 6. Статус-бары
        int status_y = 26;
        mvprintw(status_y++, 4, "Статус системы:");
        ui_print_status("НОРМАЛЬНО", 0);
        mvprintw(status_y++, 6, "Температура в норме");

        ui_print_status("ВНИМАНИЕ", 1);
        mvprintw(status_y++, 6, "Высокая нагрузка CPU");

        ui_print_status("КРИТИЧНО", 2);
        mvprintw(status_y++, 6, "Перегрев процессора");

        refresh();
        sleep(5);

        // Демонстрация переключения тем
        clear();
        ui_theme_set_theme(THEME_BTOP_NEON);
        mvprintw(2, cols/2-15, "🎨 Демонстрация тем");
        mvprintw(4, 4, "Текущая тема: %s", current_theme.name);

        ui_draw_gradient_bar(6, 4, 40, 70, COLOR_BTOP_CPU_HIGH);
        mvprintw(6, 46, "Неоновая тема");

        mvprintw(10, 4, "Нажмите любую клавишу для продолжения...");
        refresh();
        getch();

        ui_theme_set_theme(THEME_BTOP_MATRIX);
        clear();
        mvprintw(2, cols/2-15, "🎨 Матричная тема");
        mvprintw(4, 4, "Текущая тема: %s", current_theme.name);

        ui_draw_gradient_bar(6, 4, 40, 70, COLOR_BTOP_CPU_HIGH);
        mvprintw(6, 46, "Монохромный дизайн");

        mvprintw(10, 4, "Нажмите любую клавишу для выхода...");
        refresh();
        getch();

    } else {
        printw("Терминал не поддерживает цвета. Демонстрация невозможна.\n");
        printw("Нажмите любую клавишу для выхода...");
        getch();
    }

    endwin();
}

int main() {
    printf("🎨 ЗАПУСК ДЕМОНСТРАЦИИ НОВОГО ДИЗАЙНА\n");
    printf("════════════════════════════════════\n\n");

    printf("Этот демонстратор покажет:\n");
    printf("  ✓ Современный дизайн в стиле btop\n");
    printf("  ✓ Градиентные progress-бары\n");
    printf("  ✓ Анимированные элементы\n");
    printf("  ✓ Множественные темы интерфейса\n");
    printf("  ✓ Адаптивную компоновку\n");
    printf("  ✓ Unicode визуальные элементы\n\n");

    printf("Запуск демонстрации...\n");
    sleep(2);

    demo_interface_design();

    printf("\n🎯 Демонстрация завершена!\n\n");

    printf("Новый дизайн включает:\n");
    printf("  🌑 Темная тема (btop-dark) - основная\n");
    printf("  ⚡ Неоновая тема (btop-neon) - яркая\n");
    printf("  🟢 Матричная тема (btop-matrix) - стиль matrix\n");
    printf("  ☀️ Светлая тема (btop-light) - для светлых терминалов\n\n");

    printf("Особенности дизайна:\n");
    printf("  • Градиентные progress-бары с анимацией\n");
    printf("  • Современная типографика и рамки\n");
    printf("  • Адаптивная компоновка для разных терминалов\n");
    printf("  • Цветовая индикация нагрузки\n");
    printf("  • Unicode иконки и символы\n");
    printf("  • Единообразные элементы управления\n");

    return 0;
}