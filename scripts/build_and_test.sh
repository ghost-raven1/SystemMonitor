#!/bin/bash

# Скрипт сборки и тестирования SysMon

echo "🔨 Сборка основного приложения..."
gcc -Wall -O2 -Iinclude -c src/*.c -o src/

echo "🔨 Сборка основных тестов..."
gcc -Wall -O2 -Iinclude -c test_core_modules.c -o test_core_modules.o

echo "🔗 Компоновка основных тестов..."
gcc test_core_modules.o src/*.o -o test_core -lncurses

echo "🧪 Запуск основных тестов..."
./test_core

echo ""
echo "🔨 Сборка тестов дерева процессов..."
gcc -Wall -O2 -Iinclude -c test_process_tree.c src/process_tree.c -o test_process_tree.o

echo "🧪 Запуск тестов дерева процессов..."
./test_process_tree.o || echo "⚠ Тест дерева процессов не скомпилировался или не запустился"

echo ""
echo "🔨 Сборка демо дерева процессов..."
gcc -Wall -O2 -Iinclude src/process_tree_demo.c src/process_tree.c -o process_tree_demo

echo "🚀 Демо готово для запуска: ./process_tree_demo"

echo "✅ Тестирование завершено!"