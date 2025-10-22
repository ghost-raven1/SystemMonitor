/**
 * process_tree_demo.c - Демонстрация использования модуля анализа дерева процессов
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "process_tree.h"

int main() {
    printf("🌳 Демонстрация модуля анализа дерева процессов\n");
    printf("═══════════════════════════════════════════════\n\n");

    // Инициализируем дерево процессов
    process_tree_t tree;
    if (process_tree_init(&tree) != 0) {
        printf("Ошибка инициализации дерева процессов\n");
        return 1;
    }

    printf("Построение дерева процессов...\n");

    // Строим дерево
    if (process_tree_build(&tree) != 0) {
        printf("Ошибка построения дерева процессов\n");
        process_tree_destroy(&tree);
        return 1;
    }

    printf("✓ Дерево построено успешно!\n\n");

    // Показываем основную информацию
    if (tree.root) {
        printf("Корневой процесс:\n");
        printf("  PID: %d\n", tree.root->pid);
        printf("  Имя: %s\n", tree.root->name);
        printf("  Пользователь: %s\n", tree.root->user);
        printf("  Состояние: %s\n", process_state_to_string(tree.root->state[0]));
        printf("  Потоки: %d\n", tree.root->thread_count);
        printf("  Всего процессов в дереве: %d\n", tree.root->tree_size);
        printf("  Максимальная глубина: %d\n", tree.root->depth);

        printf("\nИспользование ресурсов:\n");
        printf("  CPU: %.1f%%\n", tree.root->cpu_percent);
        printf("  Память: %.1f%%\n", tree.root->memory_percent);
        printf("  RSS: %lu KB\n", tree.root->rss);
        printf("  VSZ: %lu KB\n", tree.root->vsz);

        printf("\nСуммарно для всего дерева:\n");
        printf("  CPU: %.1f%%\n", tree.root->subtree_cpu);
        printf("  Память: %.1f%%\n", tree.root->subtree_memory);
    }

    printf("\nЭкспорт дерева процессов...\n");

    // Экспортируем в различные форматы
    if (process_tree_export_txt(&tree, "process_tree_demo.txt") == 0) {
        printf("✓ Экспортировано в process_tree_demo.txt\n");
    }

    if (process_tree_export_json(&tree, "process_tree_demo.json") == 0) {
        printf("✓ Экспортировано в process_tree_demo.json\n");
    }

    if (process_tree_export_dot(&tree, "process_tree_demo.dot") == 0) {
        printf("✓ Экспортировано в process_tree_demo.dot (для Graphviz)\n");
    }

    printf("\nASCII визуализация дерева:\n");
    printf("═══════════════════════════════════════════════\n");

    // Показываем простую ASCII визуализацию
    process_tree_draw_ascii(&tree, 5);

    // Очищаем память
    process_tree_destroy(&tree);

    printf("\nДемонстрация завершена!\n");
    printf("Файлы экспорта:\n");
    printf("  - process_tree_demo.txt - текстовый формат\n");
    printf("  - process_tree_demo.json - JSON формат\n");
    printf("  - process_tree_demo.dot - Graphviz формат\n");
    printf("\nДля визуализации DOT файла используйте:\n");
    printf("  dot -Tpng process_tree_demo.dot -o process_tree.png\n");

    return 0;
}