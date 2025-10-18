#!/bin/bash

# Демонстрационный скрипт для SystemMonitor v2.0
# Показывает все новые возможности программы

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         🖥️  SystemMonitor v2.0 - Демонстрация              ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}📋 Проверка установки...${NC}"
echo ""

# Проверка наличия исполняемого файла
if [ ! -f "./sysmon" ]; then
    echo -e "${YELLOW}⚠️  Программа не скомпилирована. Компилируем...${NC}"
    make clean && make
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Ошибка компиляции!${NC}"
        exit 1
    fi
    echo -e "${GREEN}✅ Компиляция успешна!${NC}"
fi

echo ""
echo -e "${CYAN}🎯 Возможности программы:${NC}"
echo ""

echo "1. ${GREEN}Красивый интерфейс${NC} с Unicode рамками и иконками"
echo "2. ${GREEN}Графики истории${NC} загрузки CPU и памяти (60 секунд)"
echo "3. ${GREEN}Управление процессами${NC} - просмотр и завершение"
echo "4. ${GREEN}Детальная информация${NC} о системе:"
echo "   • Температуры всех ядер CPU"
echo "   • Разбивка использования памяти"
echo "   • SMART статус дисков"
echo "   • Информация о батарее с циклами"
echo "   • Мониторинг GPU"
echo "   • Список USB устройств"
echo "5. ${GREEN}Система оповещений${NC} об аномалиях и перегреве"
echo "6. ${GREEN}Настраиваемые пороги${NC} через переменные окружения"
echo "7. ${CYAN}НОВОЕ:${NC} ${GREEN}Расширенная погода${NC} с детальной информацией"
echo "8. ${CYAN}НОВОЕ:${NC} ${GREEN}Мониторинг сетевых соединений${NC} в реальном времени"
echo "9. ${CYAN}НОВОЕ:${NC} ${GREEN}Быстрые системные действия${NC} - очистка, перезапуск служб"
echo "10. ${CYAN}НОВОЕ:${NC} ${GREEN}Интеграция с Docker${NC} - мониторинг контейнеров"
echo "11. ${CYAN}НОВОЕ:${NC} ${GREEN}Системные рекомендации${NC} по оптимизации"

echo ""
echo -e "${CYAN}⚙️  Режимы запуска:${NC}"
echo ""

echo -e "${PURPLE}1. Обычный режим:${NC}"
echo "   ./sysmon"
echo ""

echo -e "${PURPLE}2. Безопасный режим (без внешних вызовов):${NC}"
echo "   SYSMON_SAFE=1 ./sysmon"
echo ""

echo -e "${PURPLE}3. Режим отладки:${NC}"
echo "   SYSMON_DEBUG=1 ./sysmon 2>debug.log"
echo ""

echo -e "${PURPLE}4. С настройкой порогов:${NC}"
echo "   SYSMON_THR_CPU=80 SYSMON_THR_MEM=90 SYSMON_THR_TEMP=75 ./sysmon"
echo ""

echo -e "${PURPLE}5. С логированием в CSV:${NC}"
echo "   SYSMON_CSV=metrics.csv ./sysmon"
echo ""

echo -e "${PURPLE}6. С отключенными модулями:${NC}"
echo "   SYSMON_NO_BAT=1 SYSMON_NO_GPU=1 ./sysmon"
echo ""

echo -e "${CYAN}🎮 Горячие клавиши:${NC}"
echo ""
echo "  ${YELLOW}Главный экран:${NC}"
echo "    [P] - Список процессов"
echo "    [N] - Сетевые соединения ${CYAN}(НОВОЕ)${NC}"
echo "    [A] - Быстрые действия ${CYAN}(НОВОЕ)${NC}"
echo "    [W] - Подробная погода ${CYAN}(НОВОЕ)${NC}"
echo "    [C] - Docker контейнеры ${CYAN}(НОВОЕ)${NC}"
echo "    [S] - Сохранить снимок"
echo "    [R] - Перезагрузка конфигурации"
echo "    [D] - Диагностика системы"
echo "    [Q] - Выход"
echo ""
echo "  ${YELLOW}Экран процессов:${NC}"
echo "    [↑/↓] - Навигация"
echo "    [K] - Завершить процесс"
echo "    [S] - Изменить сортировку"
echo "    [/] - Фильтр по имени"
echo "    [PgUp/PgDn] - Страницы"
echo "    [Q/ESC] - Назад"
echo ""
echo "  ${YELLOW}Новые быстрые действия:${NC}"
echo "    • Очистка системного кэша"
echo "    • Перезапуск сети"
echo "    • Освобождение памяти"
echo "    • И многое другое..."

echo ""
echo "════════════════════════════════════════════════════════════"
echo ""

# Проверка зависимостей
echo -e "${CYAN}📦 Проверка опциональных зависимостей:${NC}"
echo ""

check_command() {
    if command -v $1 &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $1 установлен"
        return 0
    else
        echo -e "  ${YELLOW}✗${NC} $1 не найден $2"
        return 1
    fi
}

check_command "smartctl" "(установите: brew install smartmontools)"
check_command "curl" "(для погоды)"
check_command "iostat" "(встроенная утилита macOS)"
check_command "networksetup" "(встроенная утилита macOS)"

echo ""
echo "════════════════════════════════════════════════════════════"
echo ""

# Запуск демонстрации
echo -e "${CYAN}🚀 Запуск демонстрации?${NC}"
echo ""
echo "Выберите режим:"
echo "  1) Обычный режим"
echo "  2) Безопасный режим"
echo "  3) Режим отладки"
echo "  4) Демо с низкими порогами (для тестирования оповещений)"
echo "  5) ${CYAN}Режим с погодой${NC} (требуется интернет)"
echo "  6) ${CYAN}Режим с Docker${NC} (требуется Docker)"
echo "  7) ${CYAN}Полнофункциональный режим${NC} (все функции)"
echo "  8) Выход"
echo ""
read -p "Ваш выбор (1-8): " choice

case $choice in
    1)
        echo -e "${GREEN}Запуск в обычном режиме...${NC}"
        sleep 1
        ./sysmon
        ;;
    2)
        echo -e "${GREEN}Запуск в безопасном режиме...${NC}"
        sleep 1
        SYSMON_SAFE=1 ./sysmon
        ;;
    3)
        echo -e "${GREEN}Запуск в режиме отладки (лог в debug_demo.log)...${NC}"
        sleep 1
        SYSMON_DEBUG=1 ./sysmon 2>debug_demo.log
        echo -e "${CYAN}Лог сохранен в debug_demo.log${NC}"
        ;;
    4)
        echo -e "${GREEN}Запуск с низкими порогами для демонстрации оповещений...${NC}"
        echo -e "${YELLOW}Пороги: CPU=30%, MEM=40%, TEMP=50°C${NC}"
        sleep 2
        SYSMON_THR_CPU=30 SYSMON_THR_MEM=40 SYSMON_THR_TEMP=50 ./sysmon
        ;;
    5)
        echo -e "${GREEN}Запуск с расширенной погодой...${NC}"
        echo -e "${YELLOW}Введите город (или оставьте пустым для автоопределения):${NC}"
        read city
        if [ ! -z "$city" ]; then
            export SYSMON_WEATHER_CITY="$city"
        fi
        sleep 1
        ./sysmon
        ;;
    6)
        echo -e "${GREEN}Запуск с мониторингом Docker...${NC}"
        if ! command -v docker &> /dev/null; then
            echo -e "${RED}Docker не установлен!${NC}"
            echo "Установите Docker Desktop с https://www.docker.com/products/docker-desktop"
            exit 1
        fi
        sleep 1
        SYSMON_DOCKER_ENABLED=1 ./sysmon
        ;;
    7)
        echo -e "${GREEN}Запуск в полнофункциональном режиме...${NC}"
        echo -e "${CYAN}Включены все возможности:${NC}"
        echo "  • Расширенная погода"
        echo "  • Мониторинг сетевых соединений"
        echo "  • Быстрые системные действия"
        echo "  • Docker интеграция"
        echo "  • Системные рекомендации"
        sleep 2
        SYSMON_FULL_FEATURES=1 ./sysmon
        ;;
    8)
        echo -e "${BLUE}До свидания!${NC}"
        exit 0
        ;;
    *)
        echo -e "${RED}Неверный выбор!${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}✨ Спасибо за использование SystemMonitor v2.0!${NC}"
echo ""
echo -e "${CYAN}💡 Совет:${NC} Попробуйте новые функции:"
echo "  • Нажмите [W] для детальной погоды"
echo "  • Нажмите [N] для просмотра сетевых соединений"
echo "  • Нажмите [A] для быстрых системных действий"
echo ""
