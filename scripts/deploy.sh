#!/bin/bash

# ============================================================================
# Скрипт развертывания SystemMonitor
# Подготавливает production-ready артефакты и проверяет готовность к развертыванию
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

# Конфигурация развертывания
DEPLOY_DIR="deploy"
BUILD_TYPE="${BUILD_TYPE:-release}"
VERSION="${VERSION:-$(date +%Y%m%d-%H%M%S)}"
ARTIFACTS_DIR="$DEPLOY_DIR/artifacts"

# Создание структуры директорий для развертывания
create_deploy_structure() {
    log "Создание структуры директорий для развертывания..."

    mkdir -p "$DEPLOY_DIR"
    mkdir -p "$ARTIFACTS_DIR/bin"
    mkdir -p "$DEPLOY_DIR/config"
    mkdir -p "$DEPLOY_DIR/docs"
    mkdir -p "$DEPLOY_DIR/test-reports"

    success "Структура директорий создана"
}

# Сборка основного приложения
build_application() {
    log "Сборка основного приложения ($BUILD_TYPE)..."

    local build_flags="-Wall -Wextra -O2"
    if [ "$BUILD_TYPE" = "debug" ]; then
        build_flags="-Wall -Wextra -g -O0 -DDEBUG"
    fi

    # Очистка предыдущих сборок
    make clean

    # Сборка с оптимизацией
    if CFLAGS="$build_flags" make all; then
        success "Основное приложение собрано"
    else
        error "Ошибка сборки основного приложения"
        exit 1
    fi
}

# Сборка и проверка тестового фреймворка
build_and_test_framework() {
    log "Сборка и проверка тестового фреймворка..."

    # Полная проверка качества
    if make -C test-framework quality; then
        success "Тестовый фреймворк проверен и готов"
    else
        error "Ошибка проверки тестового фреймворка"
        exit 1
    fi
}

# Создание артефактов тестирования
create_test_artifacts() {
    log "Создание артефактов тестирования..."

    # Генерация полного отчета покрытия
    make test-coverage > /dev/null 2>&1 || warning "Ошибка генерации покрытия"

    if [ -d test-framework/coverage-html ]; then
        cp -r test-framework/coverage-html "$DEPLOY_DIR/test-reports/"
        success "Отчет покрытия скопирован"
    fi

    if [ -d test-results ]; then
        cp -r test-results "$DEPLOY_DIR/"
        success "Результаты тестов скопированы"
    fi
}

# Создание документации
generate_documentation() {
    log "Генерация документации..."

    if command -v doxygen &> /dev/null; then
        if [ -f test-framework/Doxyfile ]; then
            cd test-framework && doxygen Doxyfile && cd ..
            if [ -d test-framework/docs ]; then
                cp -r test-framework/docs "$DEPLOY_DIR/"
                success "Документация сгенерирована и скопирована"
            fi
        else
            warning "Doxyfile не найден - документация не сгенерирована"
        fi
    else
        warning "Doxygen не установлен - документация не сгенерирована"
    fi
}

# Создание конфигурационных файлов для различных сред
create_config_files() {
    log "Создание конфигурационных файлов..."

    # Production конфигурация
    cat > "$DEPLOY_DIR/config/production.ini" << EOF
# Production configuration for SystemMonitor
[system]
log_level = WARNING
daemon_mode = true

[monitoring]
enable_prometheus = true
enable_grafana = true
collection_interval = 30

[ui]
theme = dark
refresh_rate = 1000
EOF

    # Development конфигурация
    cat > "$DEPLOY_DIR/config/development.ini" << EOF
# Development configuration for SystemMonitor
[system]
log_level = DEBUG
daemon_mode = false

[monitoring]
enable_prometheus = false
enable_grafana = false
collection_interval = 5

[ui]
theme = light
refresh_rate = 500
EOF

    # Тестирование конфигурация
    cat > "$DEPLOY_DIR/config/testing.ini" << EOF
# Testing configuration for SystemMonitor
[system]
log_level = INFO
daemon_mode = false

[monitoring]
enable_prometheus = false
enable_grafana = false
collection_interval = 1

[ui]
theme = minimal
refresh_rate = 200
EOF

    success "Конфигурационные файлы созданы"
}

# Создание скриптов запуска
create_startup_scripts() {
    log "Создание скриптов запуска..."

    # Скрипт запуска для Linux
    cat > "$DEPLOY_DIR/bin/start-sysmon.sh" << 'EOF'
#!/bin/bash
# Скрипт запуска SystemMonitor

CONFIG_FILE="${CONFIG_FILE:-../config/production.ini}"
LOG_FILE="${LOG_FILE:-/var/log/sysmon.log}"

# Проверка прав доступа
if [ "$EUID" -eq 0 ]; then
    echo "Запуск SystemMonitor с правами root..."
else
    echo "Предупреждение: SystemMonitor запущен без прав root"
    echo "Некоторые функции мониторинга могут быть недоступны"
fi

# Создание директории для логов
mkdir -p "$(dirname "$LOG_FILE")"

# Запуск приложения
exec ./sysmon --config "$CONFIG_FILE" --log "$LOG_FILE" "$@"
EOF

    # Скрипт остановки
    cat > "$DEPLOY_DIR/bin/stop-sysmon.sh" << 'EOF'
#!/bin/bash
# Скрипт остановки SystemMonitor

PID_FILE="${PID_FILE:-/var/run/sysmon.pid}"

if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "Остановка SystemMonitor (PID: $PID)..."
        kill "$PID"
        sleep 2

        if kill -0 "$PID" 2>/dev/null; then
            echo "Принудительная остановка..."
            kill -9 "$PID"
        fi

        rm -f "$PID_FILE"
        echo "SystemMonitor остановлен"
    else
        echo "SystemMonitor не запущен"
        rm -f "$PID_FILE"
    fi
else
    echo "PID файл не найден - SystemMonitor не запущен"
fi
EOF

    # Скрипт статуса
    cat > "$DEPLOY_DIR/bin/status-sysmon.sh" << 'EOF'
#!/bin/bash
# Скрипт проверки статуса SystemMonitor

PID_FILE="${PID_FILE:-/var/run/sysmon.pid}"

if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "SystemMonitor запущен (PID: $PID)"
        ps -p "$PID" -o pid,ppid,cmd,etime,pcpu,pmem --no-headers
        exit 0
    else
        echo "SystemMonitor не запущен (PID файл существует, но процесс не найден)"
        exit 1
    fi
else
    echo "SystemMonitor не запущен"
    exit 1
fi
EOF

    # Установка прав выполнения
    chmod +x "$DEPLOY_DIR/bin/"*.sh

    success "Скрипты запуска созданы"
}

# Создание архива для развертывания
create_deployment_package() {
    log "Создание пакета для развертывания..."

    local package_name="systemmonitor-$VERSION-$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m)"

    # Создание tar архива
    cd "$DEPLOY_DIR"
    tar -czf "../${package_name}.tar.gz" .

    if [ -f "../${package_name}.tar.gz" ]; then
        success "Пакет создан: ${package_name}.tar.gz"
        log "Размер пакета: $(du -h "../${package_name}.tar.gz" | cut -f1)"
    else
        error "Ошибка создания пакета"
        exit 1
    fi
}

# Проверка готовности к развертыванию
verify_deployment() {
    log "Проверка готовности к развертыванию..."

    local errors=0

    # Проверка наличия основного исполняемого файла
    if [ ! -f "$ARTIFACTS_DIR/bin/sysmon" ]; then
        error "Основной исполняемый файл не найден"
        ((errors++))
    fi

    # Проверка наличия тестового фреймворка
    if [ ! -f "$DEPLOY_DIR/test-framework/build/libtest_framework.a" ]; then
        warning "Библиотека тестового фреймворка не найдена"
    fi

    # Проверка наличия конфигурационных файлов
    if [ ! -f "$DEPLOY_DIR/config/production.ini" ]; then
        error "Production конфигурация не найдена"
        ((errors++))
    fi

    # Проверка наличия скриптов запуска
    if [ ! -x "$DEPLOY_DIR/bin/start-sysmon.sh" ]; then
        error "Скрипт запуска не найден или неисполняем"
        ((errors++))
    fi

    # Проверка отчетов
    if [ ! -d "$DEPLOY_DIR/test-reports" ]; then
        warning "Отчеты тестирования не найдены"
    fi

    if [ $errors -eq 0 ]; then
        success "Проверка готовности пройдена"
        return 0
    else
        error "Найдены ошибки готовности ($errors)"
        return 1
    fi
}

# Создание отчета о развертывании
generate_deployment_report() {
    log "Генерация отчета о развертывании..."

    local report_file="$DEPLOY_DIR/deployment-report-$VERSION.md"

    cat > "$report_file" << EOF
# Отчет о развертывании SystemMonitor v$VERSION

## Информация о сборке
- Дата сборки: $(date)
- Версия: $VERSION
- Тип сборки: $BUILD_TYPE
- Платформа: $(uname -s) $(uname -m)
- Компилятор: $(gcc --version | head -1)

## Содержимое пакета
- Основное приложение: $(ls -la $ARTIFACTS_DIR/bin/ 2>/dev/null | wc -l) файлов
- Конфигурационные файлы: $(ls -la $DEPLOY_DIR/config/ 2>/dev/null | wc -l) файлов
- Документация: $(ls -la $DEPLOY_DIR/docs/ 2>/dev/null | wc -l) файлов
- Отчеты тестирования: $(ls -la $DEPLOY_DIR/test-reports/ 2>/dev/null | wc -l) файлов

## Конфигурации
- Production: production.ini
- Development: development.ini
- Testing: testing.ini

## Скрипты запуска
- start-sysmon.sh - запуск приложения
- stop-sysmon.sh - остановка приложения
- status-sysmon.sh - проверка статуса

## Проверка качества
$(if [ -f $DEPLOY_DIR/test-results/ci-report-*.md ]; then echo "✅ Проверка качества пройдена"; else echo "⚠️ Проверка качества не выполнялась"; fi)

## Покрытие кода
$(if [ -d $DEPLOY_DIR/test-reports/coverage-html ]; then echo "✅ HTML отчет покрытия доступен"; else echo "⚠️ Отчет покрытия недоступен"; fi)

## Готовность к развертыванию
$(if [ -f artifacts/bin/sysmon ] && [ -f config/production.ini ] && [ -x bin/start-sysmon.sh ]; then echo "✅ Готов к развертыванию"; else echo "❌ Не готов к развертыванию"; fi)
EOF

    success "Отчет о развертывании создан: $report_file"
}

# Основная функция
main() {
    log "Начинаем подготовку к развертыванию SystemMonitor v$VERSION"

    # Создание структуры
    create_deploy_structure

    # Сборка приложения
    build_application

    # Копирование артефактов
    cp sysmon "$ARTIFACTS_DIR/bin/"

    # Сборка и проверка тестового фреймворка
    build_and_test_framework

    # Создание артефактов тестирования
    create_test_artifacts

    # Генерация документации
    generate_documentation

    # Создание конфигурационных файлов
    create_config_files

    # Создание скриптов запуска
    create_startup_scripts

    # Проверка готовности
    verify_deployment || {
        error "Развертывание не готово"
        exit 1
    }

    # Генерация отчета
    generate_deployment_report

    # Создание пакета
    create_deployment_package

    success "Подготовка к развертыванию завершена!"
    log "Пакет готов: $(pwd)/systemmonitor-$VERSION-$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m).tar.gz"
    log "Структура развертывания: $(pwd)/$DEPLOY_DIR/"

    # Выход с кодом успеха
    exit 0
}

# Запуск основной функции с обработкой параметров
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="debug"
            shift
            ;;
        --version)
            VERSION="$2"
            shift 2
            ;;
        --help)
            echo "Использование: $0 [--debug] [--version VERSION]"
            echo ""
            echo "Опции:"
            echo "  --debug      Сборка в режиме отладки"
            echo "  --version    Установка версии (по умолчанию: timestamp)"
            echo "  --help       Показать эту справку"
            exit 0
            ;;
        *)
            error "Неизвестный параметр: $1"
            echo "Используйте --help для получения справки"
            exit 1
            ;;
    esac
done

main "$@"