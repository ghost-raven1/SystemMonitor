#!/bin/bash

# ============================================================================
# Быстрый запуск тестов SystemMonitor
# Позволяет запускать конкретные категории тестов или отдельные тестовые файлы
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

# Показать справку
show_help() {
    cat << EOF
Использование: $0 [ОПЦИИ] [КАТЕГОРИЯ|ФАЙЛ]

Категории тестов:
  unit         - Unit тесты всех компонентов
  integration  - Интеграционные тесты всех компонентов
  core         - Core компоненты (unit + integration)
  platform     - Platform компоненты (unit + integration)
  ui           - UI компоненты (unit + integration)
  system       - System интеграционные тесты
  all          - Все тесты

Опции:
  --fast       - Быстрый режим (только сборка и запуск)
  --verbose    - Подробный вывод
  --coverage   - Генерация отчета покрытия
  --help       - Показать эту справку

Примеры:
  $0 unit core              # Unit тесты core компонентов
  $0 --coverage all         # Все тесты с отчетом покрытия
  $0 --fast platform        # Быстрое тестирование platform компонентов
  $0 test_core_modules.c     # Тестирование конкретного файла

EOF
}

# Проверка наличия тестового фреймворка
check_test_framework() {
    if [ ! -f "test-framework/build/libtest_framework.a" ]; then
        log "Сборка тестового фреймворка..."
        if ! make test-framework; then
            error "Ошибка сборки тестового фреймворка"
            exit 1
        fi
    fi
}

# Запуск категории тестов
run_test_category() {
    local category="$1"
    local fast_mode="$2"
    local coverage_mode="$3"

    log "Запуск категории тестов: $category"

    case "$category" in
        "unit")
            check_test_framework
            if [ "$fast_mode" = "true" ]; then
                make run-unit-tests
            else
                make unit-tests run-unit-tests
            fi
            ;;

        "integration")
            check_test_framework
            if [ "$fast_mode" = "true" ]; then
                make run-integration-tests
            else
                make integration-tests run-integration-tests
            fi
            ;;

        "core")
            check_test_framework
            if [ "$coverage_mode" = "true" ]; then
                make test-coverage
            fi
            if [ "$fast_mode" = "true" ]; then
                make run-core-tests
            else
                make core-tests run-core-tests
            fi
            ;;

        "platform")
            check_test_framework
            if [ "$fast_mode" = "true" ]; then
                make run-platform-tests
            else
                make platform-tests run-platform-tests
            fi
            ;;

        "ui")
            check_test_framework
            if [ "$fast_mode" = "true" ]; then
                make run-ui-tests
            else
                make ui-tests run-ui-tests
            fi
            ;;

        "system")
            check_test_framework
            if [ "$fast_mode" = "true" ]; then
                make run-system-tests
            else
                make system-tests run-system-tests
            fi
            ;;

        "all")
            check_test_framework
            if [ "$coverage_mode" = "true" ]; then
                make test-coverage
            fi
            if [ "$fast_mode" = "true" ]; then
                make run-unit-tests run-integration-tests
            else
                make test-all
            fi
            ;;

        *)
            error "Неизвестная категория: $category"
            show_help
            exit 1
            ;;
    esac
}

# Запуск конкретного тестового файла
run_specific_test() {
    local test_file="$1"
    local coverage_mode="$2"

    if [ ! -f "$test_file" ]; then
        error "Тестовый файл не найден: $test_file"
        exit 1
    fi

    log "Запуск конкретного теста: $test_file"

    check_test_framework

    # Определение имени исполняемого файла
    local test_name=$(basename "$test_file" .c)
    local test_binary="bin/test_${test_name}"

    # Сборка конкретного теста
    if make "$test_binary"; then
        success "Тест собран: $test_binary"
    else
        error "Ошибка сборки теста"
        exit 1
    fi

    # Запуск теста
    log "Выполнение теста..."
    if [ "$coverage_mode" = "true" ]; then
        if [ -f "$test_binary" ]; then
            ./"$test_binary"
        else
            error "Исполняемый файл теста не найден"
            exit 1
        fi
    else
        if ./"$test_binary" > "test-results/${test_name}.log" 2>&1; then
            success "Тест пройден: $test_file"
            if [ -f "test-results/${test_name}.log" ]; then
                log "Результаты сохранены в test-results/${test_name}.log"
            fi
        else
            error "Тест провален: $test_file"
            if [ -f "test-results/${test_name}.log" ]; then
                log "Детали ошибки в test-results/${test_name}.log"
            fi
            exit 1
        fi
    fi
}

# Основная функция
main() {
    local fast_mode="false"
    local verbose_mode="false"
    local coverage_mode="false"
    local target=""

    # Обработка параметров
    while [[ $# -gt 0 ]]; do
        case $1 in
            --fast)
                fast_mode="true"
                shift
                ;;
            --verbose)
                verbose_mode="true"
                shift
                ;;
            --coverage)
                coverage_mode="true"
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            -*)
                error "Неизвестный параметр: $1"
                show_help
                exit 1
                ;;
            *)
                if [ -z "$target" ]; then
                    target="$1"
                else
                    error "Слишком много параметров"
                    show_help
                    exit 1
                fi
                shift
                ;;
        esac
done

    # Если цель не указана, показать справку
    if [ -z "$target" ]; then
        show_help
        exit 0
    fi

    # Создание директории для результатов
    mkdir -p test-results bin

    # Включение подробного вывода если запрошено
    if [ "$verbose_mode" = "true" ]; then
        set -x
    fi

    log "Запуск тестового раннера..."

    # Определение типа цели и запуск соответствующих тестов
    if [[ "$target" == *.c ]]; then
        # Это конкретный файл
        run_specific_test "$target" "$coverage_mode"
    else
        # Это категория тестов
        run_test_category "$target" "$fast_mode" "$coverage_mode"
    fi

    success "Тестирование завершено успешно!"
}

# Запуск основной функции
main "$@"