#!/bin/bash

# Скрипт сборки и тестирования SysMon

echo "🔨 Сборка основного приложения..."
gcc -Wall -O2 -Iinclude -c src/*.c -o src/

echo "🔨 Сборка тестов..."
gcc -Wall -O2 -Iinclude -c test_core_modules.c -o test_core_modules.o

echo "🔗 Компоновка тестов..."
gcc test_core_modules.o src/*.o -o test_core -lncurses

echo "🧪 Запуск тестов..."
./test_core

echo "✅ Тестирование завершено!"