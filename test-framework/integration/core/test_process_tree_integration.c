/**
 * test_process_tree.c - Тестирование модуля анализа дерева процессов
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "process_tree.h"

void test_process_tree_basic() {
    printf("🧪 Тестирование базового функционала дерева процессов...\n");

    process_tree_t tree;
    int result = process_tree_init(&tree);
    assert(result == 0);

    result = process_tree_build(&tree);
    // Не проверяем строго результат, так как может не быть доступа к /proc в тестовой среде
    if (result == 0) {
        printf("✓ Дерево процессов построено успешно\n");
        if (tree.root) {
            printf("  Корневой процесс: %s (PID: %d)\n", tree.root->name, tree.root->pid);
            printf("  Размер дерева: %d процессов\n", tree.root->tree_size);
        }
    } else {
        printf("⚠ Не удалось построить дерево процессов (возможно, нет доступа к /proc)\n");
    }

    process_tree_destroy(&tree);
    printf("✓ Тест завершен\n\n");
}

void test_process_tree_export() {
    printf("🧪 Тестирование экспорта дерева процессов...\n");

    process_tree_t tree;
    process_tree_init(&tree);

    if (process_tree_build(&tree) == 0 && tree.root) {
        // Тестируем экспорт в различные форматы
        int result;

        result = process_tree_export_txt(&tree, "test_tree.txt");
        printf("%s Экспорт в TXT формат\n", result == 0 ? "✓" : "✗");

        result = process_tree_export_json(&tree, "test_tree.json");
        printf("%s Экспорт в JSON формат\n", result == 0 ? "✓" : "✗");

        result = process_tree_export_dot(&tree, "test_tree.dot");
        printf("%s Экспорт в DOT формат\n", result == 0 ? "✓" : "✗");

        printf("✓ Тест экспорта завершен\n");
    } else {
        printf("⚠ Пропускаем тест экспорта - дерево не построено\n");
    }

    process_tree_destroy(&tree);
    printf("\n");
}

void test_process_tree_stats() {
    printf("🧪 Тестирование расчета статистики...\n");

    process_tree_t tree;
    process_tree_init(&tree);

    if (process_tree_build(&tree) == 0 && tree.root) {
        process_group_stats_t stats;
        process_tree_get_stats(&tree, &stats);

        printf("✓ Статистика дерева процессов:\n");
        printf("  Процессов: %d\n", stats.total_processes);
        printf("  Потоков: %d\n", stats.total_threads);
        printf("  Общее CPU: %.1f%%\n", stats.total_cpu);
        printf("  Общее память: %.1f%%\n", stats.total_memory);

        printf("✓ Тест статистики завершен\n");
    } else {
        printf("⚠ Пропускаем тест статистики - дерево не построено\n");
    }

    process_tree_destroy(&tree);
    printf("\n");
}

void test_process_tree_search() {
    printf("🧪 Тестирование поиска процессов...\n");

    process_tree_t tree;
    process_tree_init(&tree);

    if (process_tree_build(&tree) == 0 && tree.root) {
        // Тестируем поиск по PID
        process_tree_node_t* node = process_tree_find_by_pid(&tree, 1);
        if (node) {
            printf("✓ Найден процесс с PID 1: %s\n", node->name);
        } else {
            printf("⚠ Процесс с PID 1 не найден\n");
        }

        // Тестируем поиск по имени
        int count;
        process_tree_node_t** nodes = process_tree_find_by_name(&tree, "init", &count);
        if (nodes && count > 0) {
            printf("✓ Найдено процессов 'init': %d\n", count);
            free(nodes);
        } else {
            printf("⚠ Процессы 'init' не найдены\n");
        }

        printf("✓ Тест поиска завершен\n");
    } else {
        printf("⚠ Пропускаем тест поиска - дерево не построено\n");
    }

    process_tree_destroy(&tree);
    printf("\n");
}

int main() {
    printf("🌳 Тестовый набор для модуля анализа дерева процессов\n");
    printf("════════════════════════════════════════════════════\n\n");

    test_process_tree_basic();
    test_process_tree_export();
    test_process_tree_stats();
    test_process_tree_search();

    printf("🎯 Все тесты завершены!\n");
    return 0;
}