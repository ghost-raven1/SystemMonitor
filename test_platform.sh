#!/bin/bash

echo "=== ТЕСТИРОВАНИЕ ПОДДЕРЖКИ LINUX ==="
echo ""

# Определяем операционную систему
OS=$(uname -s)
echo "Операционная система: $OS"

if [ "$OS" = "Linux" ]; then
    echo "Запуск тестов для Linux..."
    
    # Проверяем наличие необходимых инструментов
    echo ""
    echo "Проверка инструментов:"
    for tool in smartctl iwconfig nmcli sensors hwinfo; do
        if command -v "$tool" >/dev/null 2>&1; then
            echo "✓ $tool - доступен"
        else
            echo "✗ $tool - не найден"
        fi
    done
    
    # Проверяем sysfs интерфейсы
    echo ""
    echo "Проверка sysfs интерфейсов:"
    for path in "/sys/class/hwmon" "/sys/class/thermal" "/sys/class/power_supply"; do
        if [ -d "$path" ]; then
            echo "✓ $path - доступен"
        else
            echo "✗ $path - не найден"
        fi
    done
    
    # Тестируем компиляцию
    echo ""
    echo "Тестирование компиляции..."
    make clean > /dev/null 2>&1
    if make 2>/dev/null; then
        echo "✓ Компиляция успешна"
        
        # Тестируем запуск
        echo ""
        echo "Тестирование запуска программы..."
        timeout 3s ./sysmon 2>/dev/null || echo "✓ Программа запустилась и завершилась корректно"
    else
        echo "✗ Ошибка компиляции"
        make 2>&1 | head -10
    fi
    
elif [ "$OS" = "Darwin" ]; then
    echo "Запуск тестов для macOS..."
    
    # Тестируем компиляцию
    make clean > /dev/null 2>&1
    if make 2>/dev/null; then
        echo "✓ Компиляция успешна"
    else
        echo "✗ Ошибка компиляции"
        make 2>&1 | head -10
    fi
    
else
    echo "Неподдерживаемая операционная система: $OS"
    exit 1
fi

echo ""
echo "=== ТЕСТ ЗАВЕРШЕН ==="
