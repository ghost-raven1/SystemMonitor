#!/bin/bash

echo "=== СИСТЕМНЫЙ МОНИТОР - ТЕСТИРОВАНИЕ ПЛАТФОРМ ==="
echo ""

# Определяем ОС
OS=$(uname -s)
echo "🖥️  Операционная система: $OS"

# Компилируем программу
echo ""
echo "🔨 Компиляция программы..."
make clean > /dev/null 2>&1

if make > /dev/null 2>&1; then
    echo "✅ Компиляция успешна"
else
    echo "❌ Ошибка компиляции"
    make 2>&1 | head -10
    exit 1
fi

echo ""
echo "🧪 Доступные тесты функциональности:"
echo ""

if [ "$OS" = "Linux" ]; then
    echo "📋 Linux-специфичные функции:"
    echo "   • Восстановить работу батареи (исправлен segmentation fault)"
    echo "   • HWMON датчики температур через /sys/class/hwmon"
    echo "   • USB устройства через /sys/bus/usb/devices"
    echo "   • SMART мониторинг дисков через smartctl"
    echo "   • Автоматическое определение ОС в диагностике"
    echo ""
    echo "🔧 Проверка Linux инструментов:"
    
    # Проверяем наличие инструментов
    for tool in smartctl sensors iostat; do
        if command -v "$tool" >/dev/null 2>&1; then
            echo "   ✅ $tool - доступен"
        else
            echo "   ⚠️  $tool - не найден (установите пакет)"
        fi
    done
    
    echo ""
    echo "📁 Проверка sysfs интерфейсов:"
    for path in "/sys/class/hwmon" "/sys/class/thermal" "/sys/class/power_supply"; do
        if [ -d "$path" ]; then
            echo "   ✅ $path - доступен"
        else
            echo "   ❌ $path - не найден"
        fi
    done

elif [ "$OS" = "Darwin" ]; then
    echo "📋 macOS-специфичные функции:"
    echo "   • SMC датчики температур через IOKit"
    echo "   • Батарея через Core Foundation/IOKit"
    echo "   • USB устройства через IOKit"
    echo "   • Интеграция с системными инструментами macOS"
    echo ""
    echo "🔧 Проверка macOS инструментов:"
    
    # Проверяем наличие инструментов macOS
    for tool in iostat nettop; do
        if command -v "$tool" >/dev/null 2>&1 || [ -x "/usr/bin/$tool" ]; then
            echo "   ✅ $tool - доступен"
        else
            echo "   ⚠️  $tool - не найден"
        fi
    done
    
else
    echo "⚠️  Неподдерживаемая ОС: $OS"
    echo "   Будет использована базовая функциональность"
fi

echo ""
echo "🎮 Управление программой:"
echo "   Запуск: ./sysmon"
echo "   Горячие клавиши:"
echo "     T - Тестирование функциональности (НОВОЕ!)"
echo "     D - Диагностика системы"
echo "     P - Процессы"
echo "     Q - Выход"
echo "     И другие..."

echo ""
echo "🚀 Готово! Теперь вы можете:"
echo "   1. Запустить ./sysmon"
echo "   2. Нажать T для тестирования функциональности"
echo "   3. Нажать D для просмотра диагностики с определением ОС"
echo ""
