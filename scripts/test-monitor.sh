#!/bin/bash

# ============================================================================
# Мониторинг и анализ результатов тестирования SystemMonitor
# ============================================================================

set -e  # Прерывать выполнение при любой ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Функция логирования
log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

error() {
    echo -e "${RED}[ERROR] $1${NC}"
}

success() {
    echo -e "${GREEN}[SUCCESS] $1${NC}"
}

warning() {
    echo -e "${YELLOW}[WARNING] $1${NC}"
}

info() {
    echo -e "${CYAN}[INFO] $1${NC}"
}

# Анализ результатов тестов
analyze_test_results() {
    log "Анализ результатов тестирования..."

    if [ ! -d "test-results" ]; then
        warning "Директория test-results не найдена"
        return 1
    fi

    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    local total_time=0

    # Анализ unit тестов
    if [ -f "test-results/unit-tests.log" ]; then
        info "Анализ unit тестов..."

        while IFS= read -r line; do
            if [[ $line =~ Результаты.*пройдено ]]; then
                local results=$(echo "$line" | grep -o '[0-9]\+/[0-9]\+')
                if [[ $results =~ ([0-9]+)/([0-9]+) ]]; then
                    local passed=${BASH_REMATCH[1]}
                    local total=${BASH_REMATCH[2]}
                    passed_tests=$((passed_tests + passed))
                    total_tests=$((total_tests + total))
                fi
            fi
        done < "test-results/unit-tests.log"
    fi

    # Анализ интеграционных тестов
    if [ -f "test-results/integration-tests.log" ]; then
        info "Анализ интеграционных тестов..."

        while IFS= read -r line; do
            if [[ $line =~ Результаты.*пройдено ]]; then
                local results=$(echo "$line" | grep -o '[0-9]\+/[0-9]\+')
                if [[ $results =~ ([0-9]+)/([0-9]+) ]]; then
                    local passed=${BASH_REMATCH[1]}
                    local total=${BASH_REMATCH[2]}
                    passed_tests=$((passed_tests + passed))
                    total_tests=$((total_tests + total))
                fi
            fi
        done < "test-results/integration-tests.log"
    fi

    # Расчет статистики
    if [ $total_tests -gt 0 ]; then
        local success_rate=$((passed_tests * 100 / total_tests))
        failed_tests=$((total_tests - passed_tests))

        echo
        echo -e "${PURPLE}=========================================="
        echo "              СТАТИСТИКА ТЕСТИРОВАНИЯ"
        echo -e "==========================================${NC}"
        echo -e "Общее количество тестов: ${CYAN}$total_tests${NC}"
        echo -e "Пройденные тесты:        ${GREEN}$passed_tests${NC}"
        echo -e "Проваленные тесты:       ${RED}$failed_tests${NC}"
        echo -e "Процент успеха:          ${success_rate}%"

        if [ $success_rate -ge 90 ]; then
            success "Качество тестирования: ОТЛИЧНОЕ"
        elif [ $success_rate -ge 75 ]; then
            info "Качество тестирования: ХОРОШЕЕ"
        elif [ $success_rate -ge 50 ]; then
            warning "Качество тестирования: УДОВЛЕТВОРИТЕЛЬНОЕ"
        else
            error "Качество тестирования: НЕУДОВЛЕТВОРИТЕЛЬНОЕ"
        fi

        echo -e "${PURPLE}==========================================${NC}"
        echo
    else
        warning "Нет данных для анализа"
    fi
}

# Анализ покрытия кода
analyze_coverage() {
    log "Анализ покрытия кода..."

    if [ -f "test-framework/coverage.log" ]; then
        info "Основные метрики покрытия:"

        # Извлечение ключевых метрик из лога покрытия
        echo -e "${CYAN}Детали покрытия:${NC}"
        tail -20 test-framework/coverage.log

    elif [ -d "test-framework/coverage-html" ]; then
        success "HTML отчет покрытия доступен в test-framework/coverage-html/"

        # Поиск основного файла отчета
        local index_file="test-framework/coverage-html/index.html"
        if [ -f "$index_file" ]; then
            info "Основные показатели покрытия:"
            grep -A 5 -B 5 "Lines:" "$index_file" | head -10 || echo "Не удалось извлечь данные"
        fi
    else
        warning "Данные покрытия недоступны"
    fi
}

# Анализ качества кода
analyze_code_quality() {
    log "Анализ качества кода..."

    # Проверка синтаксиса
    if [ -f "test-results/cppcheck.log" ]; then
        local error_count=$(grep -c "error:" test-results/cppcheck.log || echo "0")
        local warning_count=$(grep -c "warning:" test-results/cppcheck.log || echo "0")

        echo -e "${CYAN}Статический анализ кода:${NC}"
        echo "Ошибок: $error_count"
        echo "Предупреждений: $warning_count"

        if [ "$error_count" -eq "0" ]; then
            success "Ошибки статического анализа не найдены"
        else
            error "Найдены ошибки статического анализа"
        fi
    else
        warning "Результаты статического анализа недоступны"
    fi
}

# Генерация отчета мониторинга
generate_monitoring_report() {
    log "Генерация отчета мониторинга..."

    local report_file="test-results/monitoring-report-$(date +%Y%m%d-%H%M%S).md"

    cat > "$report_file" << EOF
# Отчет мониторинга тестирования - $(date)

## Обзор проекта
- Проект: SystemMonitor
- Дата анализа: $(date)
- Версия: $(git describe --tags 2>/dev/null || echo "неизвестна")

## Результаты тестирования
$(analyze_test_results | grep -E "(Общее количество|Пройденные|Проваленные|Процент успеха|Качество тестирования)" | sed 's/^/- /' || echo "Нет данных")

## Покрытие кода
$(if [ -f test-framework/coverage.log ]; then echo "Данные покрытия доступны в coverage.log"; else echo "Данные покрытия недоступны"; fi)

## Качество кода
$(if [ -f test-results/cppcheck.log ]; then echo "Результаты статического анализа доступны"; else echo "Статический анализ не выполнялся"; fi)

## Рекомендации
$(if [ -f test-results/unit-tests.log ] && [ -f test-results/integration-tests.log ]; then echo "- Продолжать регулярное тестирование"; echo "- Поддерживать уровень покрытия > 80%"; else echo "- Выполнить полный цикл тестирования"; echo "- Проанализировать причины сбоев"; fi)

## Следующие шаги
- Мониторить тренды качества кода
- Регулярно обновлять тесты
- Поддерживать документацию в актуальном состоянии
EOF

    success "Отчет мониторинга создан: $report_file"
}

# Мониторинг производительности
monitor_performance() {
    log "Анализ производительности..."

    # Проверка времени компиляции
    if [ -f "/proc/uptime" ]; then
        local uptime=$(cut -d' ' -f1 /proc/uptime)
        info "Время работы системы: ${uptime} секунд"
    fi

    # Проверка использования памяти
    if command -v free &> /dev/null; then
        info "Использование памяти:"
        free -h | head -2
    fi

    # Проверка загрузки CPU
    if command -v uptime &> /dev/null; then
        info "Загрузка системы:"
        uptime
    fi
}

# Основная функция
main() {
    echo -e "${PURPLE}"
    echo "=========================================="
    echo "     МОНИТОРИНГ ТЕСТИРОВАНИЯ"
    echo "        SystemMonitor"
    echo "=========================================="
    echo -e "${NC}"

    # Проверка наличия данных для анализа
    if [ ! -d "test-results" ] && [ ! -d "test-framework/coverage-html" ]; then
        warning "Нет данных для анализа"
        echo "Выполните тесты для получения данных мониторинга"
        echo "Примеры команд:"
        echo "  ./scripts/test-runner.sh all"
        echo "  make test-coverage"
        exit 1
    fi

    # Выполнение анализа
    analyze_test_results
    echo
    analyze_coverage
    echo
    analyze_code_quality
    echo
    monitor_performance
    echo
    generate_monitoring_report

    success "Анализ мониторинга завершен!"
    info "Детальный отчет доступен в test-results/monitoring-report-*.md"
}

# Показать справку
show_help() {
    cat << EOF
Использование: $0 [ОПЦИИ]

Мониторинг и анализ результатов тестирования SystemMonitor.

Опции:
  --report     - Генерировать только отчет (без анализа)
  --help       - Показать эту справку

Примеры:
  $0                           - Полный анализ результатов тестирования
  $0 --report                  - Генерация отчета мониторинга

EOF
}

# Обработка параметров
case "${1:-}" in
    --help)
        show_help
        exit 0
        ;;
    --report)
        generate_monitoring_report
        exit 0
        ;;
    "")
        # Нормальный режим
        ;;
    *)
        error "Неизвестный параметр: $1"
        show_help
        exit 1
        ;;
esac

# Запуск основной функции
main "$@"
