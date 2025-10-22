#!/bin/bash

# Скрипт для запуска современного интерфейса SystemMonitor

echo "🚀 Запуск современного интерфейса SystemMonitor v2.0"
echo "=================================================="

# Устанавливаем необходимые переменные окружения
export SYSMON_FORCE_INTERACTIVE=1
export SYSMON_DEBUG=1
export TERM=xterm-256color

# Проверяем наличие исполняемого файла
if [ ! -f "./sysmon" ]; then
    echo "❌ Ошибка: исполняемый файл sysmon не найден!"
    echo "Сначала соберите проект: make"
    exit 1
fi

echo "📋 Проверка переменных окружения:"
echo "   SYSMON_FORCE_INTERACTIVE=1 (принудительный интерактивный режим)"
echo "   SYSMON_DEBUG=1 (режим отладки)"
echo "   TERM=xterm-256color (терминал с поддержкой цветов)"
echo ""

echo "🔧 Запуск интерфейса..."
echo "Нажмите Ctrl+C для выхода"
echo ""

# Запускаем приложение
./sysmon