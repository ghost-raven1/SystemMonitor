#!/bin/bash

# ============================================================================
# CI/CD скрипт для автоматического запуска тестов SystemMonitor
# ============================================================================

set -e  # Прерывать выполнение при любой ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Проверка наличия необходимых инструментов
check_dependencies() {
    log "Проверка зависимостей..."

    local missing_deps=()

    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi

    if ! command -v gcc &> /dev/null; then
        missing_deps+=("gcc")
    fi

    if ! command -v lcov &> /dev/null; then
        warning "lcov не установлен - HTML отчет покрытия недоступен"
    fi

    if ! command -v cppcheck &> /dev/null; then
        warning "cppcheck не установлен - статический анализ недоступен"
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        error "Отсутствуют необходимые зависимости: ${missing_deps[*]}"
        exit 1
    fi

    success "Все зависимости проверены"
}

# Сборка проекта
build_project() {
    log "Сборка основного проекта..."

    if make all; then
        success "Основной проект собран успешно"
    else
        error "Ошибка сборки основного проекта"
        exit 1
    fi
}

# Сборка и запуск тестового фреймворка
build_test_framework() {
    log "Сборка тестового фреймворка..."

    if make test-framework; then
        success "Тестовый фреймворк собран успешно"
    else
        error "Ошибка сборки тестового фреймворка"
        exit 1
    fi
}

# Запуск всех тестов
run_all_tests() {
    log "Запуск полного набора тестов..."

    # Создание директории для результатов
    mkdir -p test-results

    # Сборка всех тестов параллельно
    if make -j$(nproc 2>/dev/null || echo 4) test-framework unit-tests integration-tests; then
        success "Все тесты собраны"
    else
        error "Ошибка сборки тестов"
        exit 1
    fi

    # Запуск unit тестов
    log "Запуск unit тестов..."
    if make run-unit-tests > test-results/unit-tests.log 2>&1; then
        success "Unit тесты пройдены"
    else
        error "Unit тесты провалены"
        return 1
    fi

    # Запуск интеграционных тестов
    log "Запуск интеграционных тестов..."
    if make run-integration-tests > test-results/integration-tests.log 2>&1; then
        success "Интеграционные тесты пройдены"
    else
        error "Интеграционные тесты провалены"
        return 1
    fi

    return 0
}

# Генерация отчетов покрытия
generate_coverage() {
    log "Генерация отчетов покрытия кода..."

    if make coverage-html > test-results/coverage.log 2>&1; then
        success "Отчеты покрытия созданы в coverage-html/"
    else
        warning "Ошибка генерации отчетов покрытия"
    fi
}

# Статический анализ кода
static_analysis() {
    log "Выполнение статического анализа кода..."

    if command -v cppcheck &> /dev/null; then
        cppcheck --enable=all --std=c11 --language=c \
                 --suppress=missingIncludeSystem \
                 -Iinclude -Itest-framework/framework/include \
                 src/ test-framework/ > test-results/cppcheck.log 2>&1
        success "Статический анализ завершен"
    else
        warning "cppcheck не установлен - пропуск статического анализа"
    fi
}

# Проверка качества кода
quality_check() {
    log "Выполнение проверки качества кода..."

    local errors=0

    # Синтаксическая проверка
    if make -C test-framework syntax-check; then
        success "Синтаксическая проверка пройдена"
    else
        error "Синтаксическая проверка провалена"
        ((errors++))
    fi

    # Проверка форматирования (если доступен clang-format)
    if command -v clang-format &> /dev/null; then
        log "Проверка форматирования кода..."
        if make -C test-framework format; then
            success "Форматирование кода выполнено"
        fi
    fi

    return $errors
}

# Создание отчета CI/CD
generate_report() {
    log "Генерация отчета CI/CD..."

    local report_file="test-results/ci-report-$(date +%Y%m%d-%H%M%S).md"

    cat > "$report_file" << EOF
# Отчет CI/CD - $(date)

## Информация о сборке
- Дата: $(date)
- Платформа: $(uname -s)
- Компилятор: $(gcc --version | head -1)
- Ядро: $(uname -r)

## Результаты тестов

### Unit тесты
$(grep -E "(Результаты|пройдено|провалено)" test-results/unit-tests.log || echo "Нет данных")

### Интеграционные тесты
$(grep -E "(Результаты|пройдено|провалено)" test-results/integration-tests.log || echo "Нет данных")

## Покрытие кода
$(if [ -d coverage-html ]; then echo "HTML отчет доступен в coverage-html/index.html"; else echo "Отчет покрытия недоступен"; fi)

## Статический анализ
$(if [ -f test-results/cppcheck.log ]; then echo "Результаты анализа в test-results/cppcheck.log"; else echo "Статический анализ не выполнялся"; fi)

## Заключение
$(if [ -f test-results/unit-tests.log ] && [ -f test-results/integration-tests.log ]; then echo "✅ Все тесты выполнены"; else echo "❌ Некоторые тесты провалены"; fi)
EOF

    success "Отчет создан: $report_file"
}

# Основная функция
main() {
    log "Начинаем CI/CD pipeline для SystemMonitor"

    # Создание директории для результатов
    mkdir -p test-results

    # Проверка зависимостей
    check_dependencies

    # Качественная проверка
    quality_check || {
        error "Проверка качества провалена"
        exit 1
    }

    # Сборка проекта
    build_project

    # Сборка тестового фреймворка
    build_test_framework

    # Запуск тестов
    run_all_tests || {
        error "Тесты провалены"
        exit 1
    }

    # Генерация отчетов
    generate_coverage
    static_analysis
    generate_report

    success "CI/CD pipeline завершен успешно!"
    log "Результаты доступны в директории test-results/"

    # Выход с кодом успеха
    exit 0
}

# Запуск основной функции
main "$@"