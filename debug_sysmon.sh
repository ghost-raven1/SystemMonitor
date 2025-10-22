#!/bin/bash

echo "Запуск SystemMonitor с отладкой..."
echo "================================="

# Запускаем программу в фоне
SYSMON_DEBUG=1 SYSMON_FORCE_INTERACTIVE=1 ./sysmon &
SYSMON_PID=$!

echo "Программа запущена с PID: $SYSMON_PID"

# Ждем 5 секунд
echo "Ожидание 5 секунд..."
sleep 5

# Завершаем программу
echo "Завершение программы..."
kill $SYSMON_PID 2>/dev/null

echo ""
echo "================================="
echo "Тестирование простого интерфейса:"

# Тестируем простой интерфейс
./simple_ui &
SIMPLE_PID=$!

echo "Простой интерфейс запущен с PID: $SIMPLE_PID"

# Ждем 3 секунды для простого интерфейса
echo "Ожидание 3 секунд..."
sleep 3

# Завершаем простой интерфейс
echo "Завершение простого интерфейса..."
kill $SIMPLE_PID 2>/dev/null

echo "Все тесты завершены."