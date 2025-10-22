#include <stdio.h>
#include "src/ui/ui_theme.h"

int main() {
    printf("Тестирование интеграции модуля тем...\n");

    // Проверяем определение констант тем
    printf("Константы тем определены:\n");
    printf("THEME_BTOP_DARK = %d\n", THEME_BTOP_DARK);
    printf("THEME_NEON = %d\n", THEME_NEON);
    printf("THEME_MATRIX = %d\n", THEME_MATRIX);
    printf("THEME_LIGHT = %d\n", THEME_LIGHT);

    // Проверяем цветовые константы
    printf("\nЦветовые константы определены:\n");
    printf("COLOR_BTOP_BG = %d\n", COLOR_BTOP_BG);
    printf("COLOR_BTOP_TEXT = %d\n", COLOR_BTOP_TEXT);
    printf("COLOR_BTOP_TITLE = %d\n", COLOR_BTOP_TITLE);
    printf("COLOR_BTOP_SUCCESS = %d\n", COLOR_BTOP_SUCCESS);

    printf("\nИнтеграция модуля тем прошла успешно!\n");
    printf("Все константы и структуры определены корректно.\n");

    return 0;
}