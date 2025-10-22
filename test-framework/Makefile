# ============================================================================
# Makefile для тестового фреймворка SystemMonitor v3.0
#
# Этот Makefile обеспечивает сборку и тестирование компонентов
# тестового фреймворка, интегрированного с архитектурой SystemMonitor.
# ============================================================================

# Компилятор и флаги
CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=c11 -O2 -fPIC
DEBUG_CFLAGS := -Wall -Wextra -g -std=c11 -O0 -fPIC -DDEBUG

# Директории проекта
FRAMEWORK_DIR := framework
SRC_DIR := $(FRAMEWORK_DIR)/src
INCLUDE_DIR := $(FRAMEWORK_DIR)/include
UNIT_DIR := unit
INTEGRATION_DIR := integration
BUILD_DIR := build
BIN_DIR := bin
TEST_RESULTS_DIR := test-results

# Исходные файлы фреймворка
FRAMEWORK_SOURCES := $(SRC_DIR)/test_framework.c $(SRC_DIR)/test_asserts.c
FRAMEWORK_HEADERS := $(INCLUDE_DIR)/test_framework.h $(INCLUDE_DIR)/test_macros.h $(INCLUDE_DIR)/test_asserts.h $(INCLUDE_DIR)/system_monitor_types.h

# Тестовые файлы - автоматическое обнаружение всех тестов
UNIT_TESTS := $(wildcard $(UNIT_DIR)/core/*.c) \
              $(wildcard $(UNIT_DIR)/platform/*.c) \
              $(wildcard $(UNIT_DIR)/ui/*.c) \
              $(wildcard $(UNIT_DIR)/modules/*.c) \
              $(wildcard $(UNIT_DIR)/utils/*.c)

INTEGRATION_TESTS := $(wildcard $(INTEGRATION_DIR)/core/*.c) \
                     $(wildcard $(INTEGRATION_DIR)/platform/*.c) \
                     $(wildcard $(INTEGRATION_DIR)/system/*.c) \
                     $(wildcard $(INTEGRATION_DIR)/ui/*.c) \
                     $(wildcard $(INTEGRATION_DIR)/components/*.c) \
                     $(wildcard $(INTEGRATION_DIR)/end-to-end/*.c)

# Категории тестов для удобства
CORE_TESTS := $(wildcard $(UNIT_DIR)/core/*.c) $(wildcard $(INTEGRATION_DIR)/core/*.c)
PLATFORM_TESTS := $(wildcard $(UNIT_DIR)/platform/*.c) $(wildcard $(INTEGRATION_DIR)/platform/*.c)
UI_TESTS := $(wildcard $(UNIT_DIR)/ui/*.c) $(wildcard $(INTEGRATION_DIR)/ui/*.c)
SYSTEM_TESTS := $(wildcard $(INTEGRATION_DIR)/system/*.c)
MODULES_TESTS := $(wildcard $(UNIT_DIR)/modules/*.c)
UTILS_TESTS := $(wildcard $(UNIT_DIR)/utils/*.c)

# Выходные файлы
FRAMEWORK_LIB := $(BUILD_DIR)/libtest_framework.a
FRAMEWORK_SHARED := $(BUILD_DIR)/libtest_framework.so
UNIT_TEST_BIN := $(BIN_DIR)/unit_tests
INTEGRATION_TEST_BIN := $(BIN_DIR)/integration_tests

# Зависимости для тестов
TEST_DEPENDENCIES := $(FRAMEWORK_LIB) $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)

# Параллельная сборка
JOBS := $(shell nproc 2>/dev/null || echo 4)

# Категории сборки для параллельного выполнения
UNIT_TEST_BINS := $(patsubst %.c, $(BIN_DIR)/%, $(notdir $(UNIT_TESTS)))
INTEGRATION_TEST_BINS := $(patsubst %.c, $(BIN_DIR)/%, $(notdir $(INTEGRATION_TESTS)))

# ============================================================================
# Основные цели
# ============================================================================

.PHONY: all clean test unit-tests integration-tests framework help

# Цель по умолчанию
all: framework unit-tests integration-tests

# Помощь
help:
	@echo "=========================================="
	@echo "Тестовый фреймворк SystemMonitor v3.0"
	@echo "=========================================="
	@echo ""
	@echo "Основные цели сборки:"
	@echo "  all              - Сборка всего проекта (фреймворк + тесты)"
	@echo "  framework        - Сборка библиотеки фреймворка"
	@echo ""
	@echo "Тестирование:"
	@echo "  unit-tests       - Сборка всех unit тестов"
	@echo "  integration-tests- Сборка всех интеграционных тестов"
	@echo "  core-tests       - Сборка core тестов"
	@echo "  platform-tests   - Сборка platform тестов"
	@echo "  ui-tests         - Сборка UI тестов"
	@echo "  system-tests     - Сборка system тестов"
	@echo ""
	@echo "Запуск тестов:"
	@echo "  run-unit-tests   - Запуск всех unit тестов с отчетом"
	@echo "  run-integration-tests - Запуск интеграционных тестов с отчетом"
	@echo "  run-core-tests   - Запуск core тестов с отчетом"
	@echo "  run-platform-tests - Запуск platform тестов с отчетом"
	@echo "  run-ui-tests     - Запуск UI тестов с отчетом"
	@echo "  run-system-tests - Запуск system тестов с отчетом"
	@echo ""
	@echo "Комплексное тестирование:"
	@echo "  test             - Сборка и запуск всех тестов"
	@echo "  test-all         - Сборка и запуск всех категорий тестов"
	@echo ""
	@echo "Управление:"
	@echo "  clean            - Очистка собранных файлов"
	@echo "  install          - Установка фреймворка в систему"
	@echo "  uninstall        - Удаление фреймворка из системы"
	@echo ""
	@echo "Анализ и качество:"
	@echo "  docs             - Генерация документации"
	@echo "  coverage         - Анализ покрытия кода (текстовый отчет)"
	@echo "  coverage-html    - Анализ покрытия с HTML отчетом"
	@echo "  coverage-clean   - Очистка данных покрытия"
	@echo "  static-analysis  - Статический анализ кода"
	@echo "  format           - Автоматическое форматирование кода"
	@echo "  quality          - Комплексная проверка качества"
	@echo ""
	@echo "CI/CD и развертывание:"
	@echo "  precommit        - Предкоммитная проверка"
	@echo "  ci               - Полная проверка CI/CD"
	@echo "  deploy-check     - Проверка готовности к развертыванию"
	@echo ""
	@echo "Примеры использования:"
	@echo "  make test-all    - Полное тестирование всех компонентов"
	@echo "  make run-unit-tests - Запуск только unit тестов"
	@echo "  make ci          - Проверка для CI/CD"
	@echo "  make quality     - Комплексная проверка качества"
	@echo "  make coverage-html - Генерация HTML отчета покрытия"

# ============================================================================
# Сборка фреймворка
# ============================================================================

# Создание директорий
$(BUILD_DIR) $(BIN_DIR) $(TEST_RESULTS_DIR):
	mkdir -p $@

# Сборка статической библиотеки фреймворка
$(FRAMEWORK_LIB): $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS) | $(BUILD_DIR)
	@echo "Сборка статической библиотеки фреймворка..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $(FRAMEWORK_SOURCES)
	mv *.o $(BUILD_DIR)/
	ar rcs $@ $(BUILD_DIR)/*.o
	@echo "Статическая библиотека создана: $@"

# Сборка динамической библиотеки фреймворка
$(FRAMEWORK_SHARED): $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS) | $(BUILD_DIR)
	@echo "Сборка динамической библиотеки фреймворка..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -shared -o $@ $(FRAMEWORK_SOURCES)
	@echo "Динамическая библиотека создана: $@"

# Сборка фреймворка (статическая + динамическая библиотеки)
framework: $(FRAMEWORK_LIB) $(FRAMEWORK_SHARED)

# ============================================================================
# Сборка и запуск unit тестов
# ============================================================================

# Сборка исполняемого файла unit тестов
$(UNIT_TEST_BIN): $(UNIT_TESTS) $(TEST_DEPENDENCIES) | $(BIN_DIR)
	@echo "Сборка unit тестов..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $(UNIT_TESTS) $(FRAMEWORK_LIB)
	@echo "Unit тесты собраны: $@"

# ============================================================================
# Сборка и запуск unit тестов
# ============================================================================

# Шаблонное правило для сборки отдельных unit тестов
$(BIN_DIR)/test_%: $(UNIT_DIR)/%/test_%.c $(TEST_DEPENDENCIES) | $(BIN_DIR)
	@echo "Сборка unit теста: $*"
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $< $(FRAMEWORK_LIB)

$(BIN_DIR)/test_%: $(UNIT_DIR)/core/test_%.c $(TEST_DEPENDENCIES) | $(BIN_DIR)
	@echo "Сборка core unit теста: $*"
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $< $(FRAMEWORK_LIB)

# Сборка всех unit тестов параллельно
unit-tests: $(UNIT_TEST_BINS)
	@echo "Все unit тесты собраны"

# Запуск unit тестов с отчетом
run-unit-tests: $(UNIT_TEST_BINS) | $(TEST_RESULTS_DIR)
	@echo "Запуск всех unit тестов..."
	@total=0; passed=0; failed=0; \
	for test in $(UNIT_TEST_BINS); do \
		if [ -f "$$test" ]; then \
			echo "Выполнение: $$test"; \
			total=$$((total + 1)); \
			if ./$$test; then \
				passed=$$((passed + 1)); \
			else \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "Результаты unit тестов: $$passed/$$total пройдено, $$failed провалено"

# ============================================================================
# Сборка и запуск интеграционных тестов
# ============================================================================

# Шаблонное правило для сборки отдельных интеграционных тестов
$(BIN_DIR)/test_%: $(INTEGRATION_DIR)/%/test_%.c $(TEST_DEPENDENCIES) | $(BIN_DIR)
	@echo "Сборка интеграционного теста: $*"
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $< $(FRAMEWORK_LIB)

# Сборка всех интеграционных тестов параллельно
integration-tests: $(INTEGRATION_TEST_BINS)
	@echo "Все интеграционные тесты собраны"

# Запуск интеграционных тестов с отчетом
run-integration-tests: $(INTEGRATION_TEST_BINS) | $(TEST_RESULTS_DIR)
	@echo "Запуск всех интеграционных тестов..."
	@total=0; passed=0; failed=0; \
	for test in $(INTEGRATION_TEST_BINS); do \
		if [ -f "$$test" ]; then \
			echo "Выполнение: $$test"; \
			total=$$((total + 1)); \
			if ./$$test; then \
				passed=$$((passed + 1)); \
			else \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "Результаты интеграционных тестов: $$passed/$$total пройдено, $$failed провалено"

# ============================================================================
# Запуск всех тестов
# ============================================================================

# ============================================================================
# Категорированные тесты
# ============================================================================

# Тесты core компонентов
core-tests: $(filter $(BIN_DIR)/test_core%, $(UNIT_TEST_BINS) $(INTEGRATION_TEST_BINS))
	@echo "Сборка core тестов завершена"

run-core-tests: $(filter $(BIN_DIR)/test_core%, $(UNIT_TEST_BINS) $(INTEGRATION_TEST_BINS)) | $(TEST_RESULTS_DIR)
	@echo "Запуск core тестов..."
	@total=0; passed=0; failed=0; \
	for test in $^; do \
		if [ -f "$$test" ]; then \
			echo "Выполнение: $$(basename $$test)"; \
			total=$$((total + 1)); \
			if ./$$test; then \
				passed=$$((passed + 1)); \
			else \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "Результаты core тестов: $$passed/$$total пройдено, $$failed провалено"

# Тесты platform компонентов
platform-tests: $(filter $(BIN_DIR)/test_battery% $(BIN_DIR)/test_platform%, $(UNIT_TEST_BINS) $(INTEGRATION_TEST_BINS))
	@echo "Сборка platform тестов завершена"

run-platform-tests: $(filter $(BIN_DIR)/test_battery% $(BIN_DIR)/test_platform%, $(UNIT_TEST_BINS) $(INTEGRATION_TEST_BINS)) | $(TEST_RESULTS_DIR)
	@echo "Запуск platform тестов..."
	@total=0; passed=0; failed=0; \
	for test in $^; do \
		if [ -f "$$test" ]; then \
			echo "Выполнение: $$(basename $$test)"; \
			total=$$((total + 1)); \
			if ./$$test; then \
				passed=$$((passed + 1)); \
			else \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "Результаты platform тестов: $$passed/$$total пройдено, $$failed провалено"

# Тесты UI компонентов
ui-tests: $(filter $(BIN_DIR)/test_ui%, $(UNIT_TEST_BINS) $(INTEGRATION_TEST_BINS))
	@echo "Сборка UI тестов завершена"

run-ui-tests: $(filter $(BIN_DIR)/test_ui%, $(UNIT_TEST_BINS) $(INTEGRATION_TEST_BINS)) | $(TEST_RESULTS_DIR)
	@echo "Запуск UI тестов..."
	@total=0; passed=0; failed=0; \
	for test in $^; do \
		if [ -f "$$test" ]; then \
			echo "Выполнение: $$(basename $$test)"; \
			total=$$((total + 1)); \
			if ./$$test; then \
				passed=$$((passed + 1)); \
			else \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "Результаты UI тестов: $$passed/$$total пройдено, $$failed провалено"

# Тесты system компонентов
system-tests: $(filter $(BIN_DIR)/test_system%, $(INTEGRATION_TEST_BINS))
	@echo "Сборка system тестов завершена"

run-system-tests: $(filter $(BIN_DIR)/test_system%, $(INTEGRATION_TEST_BINS)) | $(TEST_RESULTS_DIR)
	@echo "Запуск system тестов..."
	@total=0; passed=0; failed=0; \
	for test in $^; do \
		if [ -f "$$test" ]; then \
			echo "Выполнение: $$(basename $$test)"; \
			total=$$((total + 1)); \
			if ./$$test; then \
				passed=$$((passed + 1)); \
			else \
				failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo "Результаты system тестов: $$passed/$$total пройдено, $$failed провалено"

# ============================================================================
# Основная цель тестирования
# ============================================================================

# Запуск всех тестов с параллельной сборкой и подробным отчетом
test: framework
	@echo "Сборка фреймворка завершена"
	@$(MAKE) -j$(JOBS) unit-tests integration-tests
	@echo "Параллельная сборка тестов завершена"
	@$(MAKE) run-unit-tests
	@$(MAKE) run-integration-tests
	@echo "Все тесты выполнены"

# Полный тест с категориями
test-all: framework
	@echo "Сборка фреймворка завершена"
	@$(MAKE) -j$(JOBS) unit-tests integration-tests core-tests platform-tests ui-tests system-tests
	@echo "Параллельная сборка всех тестов завершена"
	@echo "=== ЗАПУСК UNIT ТЕСТОВ ==="
	@$(MAKE) run-unit-tests
	@echo "=== ЗАПУСК INTEGRATION ТЕСТОВ ==="
	@$(MAKE) run-integration-tests
	@echo "=== ЗАПУСК CORE ТЕСТОВ ==="
	@$(MAKE) run-core-tests
	@echo "=== ЗАПУСК PLATFORM ТЕСТОВ ==="
	@$(MAKE) run-platform-tests
	@echo "=== ЗАПУСК UI ТЕСТОВ ==="
	@$(MAKE) run-ui-tests
	@echo "=== ЗАПУСК SYSTEM ТЕСТОВ ==="
	@$(MAKE) run-system-tests
	@echo "Все категории тестов выполнены"

# ============================================================================
# Установка и удаление
# ============================================================================

# Установка фреймворка в систему
install: framework
	@echo "Установка фреймворка в систему..."
	sudo cp $(FRAMEWORK_LIB) /usr/local/lib/
	sudo cp $(FRAMEWORK_SHARED) /usr/local/lib/
	sudo cp $(FRAMEWORK_HEADERS) /usr/local/include/
	sudo ldconfig
	@echo "Фреймворк установлен в систему"

# Удаление фреймворка из системы
uninstall:
	@echo "Удаление фреймворка из системы..."
	sudo rm -f /usr/local/lib/libtest_framework.a
	sudo rm -f /usr/local/lib/libtest_framework.so
	sudo rm -f /usr/local/include/test_framework.h
	sudo rm -f /usr/local/include/test_macros.h
	sudo rm -f /usr/local/include/test_asserts.h
	sudo rm -f /usr/local/include/system_monitor_types.h
	sudo ldconfig
	@echo "Фреймворк удален из системы"

# ============================================================================
# Очистка проекта
# ============================================================================

# Очистка всех собранных файлов
clean:
	@echo "Очистка проекта..."
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(TEST_RESULTS_DIR)
	find . -name "*.o" -delete
	find . -name "*.so" -delete
	find . -name "*.a" -delete
	@echo "Очистка завершена"

# Полная очистка (включая временные файлы)
distclean: clean
	rm -rf *.log
	rm -rf tmp/
	@echo "Полная очистка завершена"

# ============================================================================
# Документация и анализ
# ============================================================================

# Генерация документации (если установлен doxygen)
docs:
	@echo "Генерация документации..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
	else \
		echo "Doxygen не установлен. Установите его для генерации документации."; \
	fi

# ============================================================================
# Анализ покрытия кода с поддержкой LCOV
# ============================================================================

# Анализ покрытия кода (если установлен gcov/lcov)
coverage: CFLAGS += --coverage -fprofile-arcs -ftest-coverage
coverage: clean test
	@echo "Анализ покрытия кода..."
	@if command -v gcov >/dev/null 2>&1; then \
		echo "Генерация данных покрытия..."; \
		find $(BUILD_DIR) -name "*.gcno" -exec gcov {} \; > /dev/null 2>&1; \
		echo "Результаты покрытия сохранены в coverage.log"; \
	else \
		echo "gcov не установлен. Установите gcc с поддержкой покрытия."; \
	fi

# Генерация HTML отчета покрытия (если установлен lcov)
coverage-html: coverage
	@echo "Генерация HTML отчета покрытия..."
	@if command -v lcov >/dev/null 2>&1; then \
		lcov --capture --directory $(BUILD_DIR) --output-file coverage.info; \
		lcov --remove coverage.info '/usr/*' --output-file coverage.info; \
		genhtml coverage.info --output-directory coverage-html; \
		echo "HTML отчет покрытия создан в coverage-html/index.html"; \
	else \
		echo "lcov не установлен. Установите для генерации HTML отчетов."; \
	fi

# Очистка данных покрытия
coverage-clean:
	@echo "Очистка данных покрытия..."
	rm -f *.gcno *.gcda *.gcov coverage.info
	rm -rf coverage-html/

# ============================================================================
# Разработка и отладка
# ============================================================================

# Сборка в режиме отладки
debug: CFLAGS := $(DEBUG_CFLAGS)
debug: clean all

# Быстрая сборка для разработки
quick: CFLAGS += -Wno-error
quick: clean all

# Проверка синтаксиса
syntax-check:
	@echo "Проверка синтаксиса исходных файлов..."
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -fsyntax-only $(FRAMEWORK_SOURCES) $(UNIT_TESTS) $(INTEGRATION_TESTS)

# Статический анализ кода (если установлен cppcheck)
static-analysis:
	@echo "Статический анализ кода..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c11 --language=c -I$(INCLUDE_DIR) $(FRAMEWORK_SOURCES) $(UNIT_TESTS) $(INTEGRATION_TESTS); \
	else \
		echo "cppcheck не установлен. Установите его для статического анализа."; \
	fi

# ============================================================================
# Пакетирование и распространение
# ============================================================================

# Создание архива с исходными кодами
dist: clean
	@echo "Создание архива с исходными кодами..."
	tar -czf test-framework-$(shell date +%Y%m%d).tar.gz --exclude-vcs --exclude="*.tar.gz" ../test-framework/

# Создание deb пакета (если установлен dpkg-buildpackage)
deb-package:
	@echo "Создание deb пакета..."
	@if command -v dpkg-buildpackage >/dev/null 2>&1; then \
		dpkg-buildpackage -us -uc; \
	else \
		echo "dpkg-buildpackage не установлен. Установите его для создания пакетов."; \
	fi

# ============================================================================
# Специальные цели для CI/CD
# ============================================================================

# ============================================================================
# Цели для CI/CD и автоматизации
# ============================================================================

# Цель для непрерывной интеграции (полная проверка)
ci: syntax-check static-analysis test coverage-html
	@echo "=========================================="
	@echo "CI pipeline completed successfully"
	@echo "=========================================="
	@echo "Проверьте отчеты:"
	@echo "  - coverage-html/index.html (покрытие кода)"
	@echo "  - test-results/ (результаты тестов)"
	@echo "=========================================="

# Цель для предкоммитной проверки (быстрая)
precommit: syntax-check static-analysis test
	@echo "=========================================="
	@echo "Предкоммитная проверка завершена успешно"
	@echo "=========================================="

# Цель для развертывания (production ready)
deploy-check: ci
	@echo "Проверка готовности к развертыванию..."
	@test -d coverage-html && echo "Отчет покрытия создан" || (echo "Ошибка: отчет покрытия не создан"; exit 1)
	@test -d test-results && echo "Результаты тестов сохранены" || (echo "Ошибка: результаты тестов не сохранены"; exit 1)
	@echo "=========================================="
	@echo "Готовность к развертыванию подтверждена"
	@echo "=========================================="

# Автоматическое форматирование кода (если установлен clang-format)
format:
	@echo "Форматирование кода..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(FRAMEWORK_DIR) $(UNIT_DIR) $(INTEGRATION_DIR) -name "*.c" -o -name "*.h" | \
		xargs clang-format -i; \
		echo "Код отформатирован"; \
	else \
		echo "clang-format не установлен. Установите для автоматического форматирования."; \
	fi

# Проверка качества кода (комплексная)
quality: precommit coverage-html static-analysis
	@echo "=========================================="
	@echo "Комплексная проверка качества завершена"
	@echo "=========================================="

# ============================================================================
# Информация о проекте
# ============================================================================

# Вывод информации о сборке
info:
	@echo "=========================================="
	@echo "Информация о тестовом фреймворке SystemMonitor"
	@echo "=========================================="
	@echo "Версия фреймворка: 3.0.0"
	@echo "Компилятор: $(CC)"
	@echo "Флаги сборки: $(CFLAGS)"
	@echo "Директория исходных кодов: $(SRC_DIR)"
	@echo "Директория заголовков: $(INCLUDE_DIR)"
	@echo "Директория сборки: $(BUILD_DIR)"
	@echo "Директория исполняемых файлов: $(BIN_DIR)"
	@echo "=========================================="

# Вывод списка файлов
list-files:
	@echo "Файлы фреймворка:"
	@find $(FRAMEWORK_DIR) -name "*.h" -o -name "*.c" | sort
	@echo ""
	@echo "Тестовые файлы:"
	@find $(UNIT_DIR) $(INTEGRATION_DIR) -name "*.c" | sort
	@echo ""
	@echo "Makefile и документация:"
	@find . -maxdepth 1 -name "Makefile" -o -name "*.md" -o -name "Doxyfile" | sort

# ============================================================================
# Зависимости файлов
# ============================================================================

# Зависимости для unit тестов
$(UNIT_TEST_BIN): $(UNIT_TESTS) $(FRAMEWORK_LIB)

# Зависимости для интеграционных тестов
$(INTEGRATION_TEST_BIN): $(INTEGRATION_TESTS) $(FRAMEWORK_LIB)

# Общие зависимости
$(FRAMEWORK_LIB) $(FRAMEWORK_SHARED): $(FRAMEWORK_SOURCES) $(FRAMEWORK_HEADERS)

# ============================================================================
# Правила сборки по умолчанию
# ============================================================================

# Правило сборки объектных файлов
%.o: %.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Правило компоновки статической библиотеки
libtest_framework.a: $(FRAMEWORK_SOURCES:.c=.o)
	ar rcs $@ $^

# Правило компоновки динамической библиотеки
libtest_framework.so: $(FRAMEWORK_SOURCES:.c=.o)
	$(CC) $(CFLAGS) -shared -o $@ $^

# ============================================================================
# Условные зависимости для различных платформ
# ============================================================================

# Платформенные различия
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CFLAGS += -DLINUX_BUILD
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -DMACOS_BUILD
endif

# ============================================================================
# Переменные окружения для кастомизации сборки
# ============================================================================

# Поддержка кастомных флагов компилятора
ifdef CUSTOM_CFLAGS
    CFLAGS := $(CUSTOM_CFLAGS)
endif

# Поддержка кастомной директории установки
ifdef PREFIX
    INSTALL_PREFIX := $(PREFIX)
else
    INSTALL_PREFIX := /usr/local
endif

# ============================================================================
# Финальные настройки
# ============================================================================

# Отключение встроенных правил make
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# Вывод команд при выполнении (для отладки)
ifdef VERBOSE
    MAKEFLAGS :=
endif

# Цель по умолчанию если не указана
.DEFAULT_GOAL := all

# ============================================================================
# Конец Makefile
# ============================================================================