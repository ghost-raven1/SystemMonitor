# Тестовый фреймворк SystemMonitor v3.0

Базовая инфраструктура тестового фреймворка, созданная в соответствии с архитектурой SystemMonitor v3.0.

## Обзор архитектуры

Тестовый фреймворк построен на принципах модульности и расширяемости архитектуры SystemMonitor:

```
test-framework/
├── framework/                 # Библиотека фреймворка
│   ├── include/              # Заголовочные файлы
│   └── src/                  # Исходные файлы реализации
├── unit/                     # Unit тесты
│   └── core/                # Тесты базовых компонентов
├── integration/             # Интеграционные тесты
│   └── system/              # Системные интеграционные тесты
└── Makefile                 # Система сборки
```

## Основные компоненты

### 1. Библиотека фреймворка (`framework/`)

#### Заголовочные файлы:
- **`test_framework.h`** - Основной API фреймворка
- **`test_macros.h`** - Макросы для удобного создания тестов
- **`test_asserts.h`** - Функции ассертов
- **`system_monitor_types.h`** - Типы данных архитектуры SystemMonitor

#### Реализация:
- **`test_framework.c`** - Основная логика фреймворка
- **`test_asserts.c`** - Реализация ассертов

### 2. Типы тестов

- **Unit тесты** - Тестирование отдельных модулей
- **Интеграционные тесты** - Тестирование взаимодействия между модулями
- **End-to-End тесты** - Полный цикл работы системы
- **Тесты производительности** - Измерение производительности
- **Стресс-тесты** - Тестирование под высокой нагрузкой

## Быстрый старт

### Сборка фреймворка

```bash
cd test-framework
make framework
```

### Запуск unit тестов

```bash
make unit-tests
```

### Запуск интеграционных тестов

```bash
make integration-tests
```

### Запуск всех тестов

```bash
make test
```

## Использование в коде

### Пример простого unit теста

```c
#include "test_framework.h"
#include "test_macros.h"

TEST_BEGIN(test_example) {
    int a = 5, b = 3;
    TEST_ASSERT_EQUAL(8, a + b, "Сложение работает неверно");
    TEST_ASSERT_TRUE(a > b, "a должно быть больше b");
}
TEST_END();

int main() {
    test_config_t config = {0};
    config.verbose = true;

    test_framework_init(&config);

    test_definition_t *test_def = test_definition_create(
        "test_example", "math", test_example, TEST_TYPE_UNIT);
    test_register(test_def);

    test_framework_run_all();
    test_framework_cleanup();

    return 0;
}
```

### Пример интеграционного теста

```c
#include "test_framework.h"
#include "test_macros.h"
#include "system_monitor_types.h"

TEST_BEGIN(test_module_integration) {
    /* Тестирование взаимодействия между модулями */

    module_t cpu_monitor = {0};
    module_t memory_monitor = {0};

    /* Инициализация модулей */
    TEST_ASSERT_SUCCESS(cpu_monitor.ops.init(&cpu_monitor.info, NULL),
                       "Инициализация CPU монитора");
    TEST_ASSERT_SUCCESS(memory_monitor.ops.init(&memory_monitor.info, NULL),
                       "Инициализация memory монитора");

    /* Проверка взаимодействия */
    TEST_ASSERT_TRUE(cpu_monitor.info.status == MODULE_STATUS_RUNNING,
                    "CPU монитор должен работать");
    TEST_ASSERT_TRUE(memory_monitor.info.status == MODULE_STATUS_RUNNING,
                    "Memory монитор должен работать");
}
TEST_END();
```

## Архитектурные особенности

### Интеграция с SystemMonitor

Фреймворк полностью интегрирован с архитектурой SystemMonitor v3.0:

- **Модульная система** - Поддержка тестирования модулей с четкими интерфейсами
- **Система событий** - Асинхронная обработка событий между тестами
- **Сбор метрик** - Мониторинг производительности и ресурсов
- **Система плагинов** - Расширяемость через плагины тестирования

### Ключевые возможности

1. **Многоуровневое тестирование**
   - Unit тесты для отдельных функций
   - Интеграционные тесты для взаимодействия модулей
   - End-to-End тесты полного функционала

2. **Производительность и мониторинг**
   - Измерение времени выполнения тестов
   - Мониторинг использования памяти
   - Анализ покрытия кода

3. **Расширяемость**
   - Плагины для кастомных ассертов
   - Кастомные обработчики событий
   - Пользовательские типы тестов

4. **Отчетность**
   - Генерация отчетов в форматах XML, JSON, текст
   - Статистика выполнения тестов
   - Детализированные логи

## Структура проекта

```
test-framework/
├── framework/
│   ├── include/
│   │   ├── test_framework.h      # Основной API
│   │   ├── test_macros.h         # Макросы тестирования
│   │   ├── test_asserts.h        # Функции ассертов
│   │   └── system_monitor_types.h # Типы архитектуры
│   └── src/
│       ├── test_framework.c      # Реализация API
│       └── test_asserts.c        # Реализация ассертов
├── unit/core/
│   └── test_example.c           # Примеры unit тестов
├── integration/system/
│   └── test_integration_example.c # Примеры интеграционных тестов
├── Makefile                     # Система сборки
└── README.md                    # Документация
```

## Цели сборки Makefile

| Цель | Описание |
|------|----------|
| `make all` | Сборка всего проекта |
| `make framework` | Сборка библиотеки фреймворка |
| `make unit-tests` | Сборка и запуск unit тестов |
| `make integration-tests` | Сборка и запуск интеграционных тестов |
| `make test` | Запуск всех тестов |
| `make clean` | Очистка собранных файлов |
| `make install` | Установка в систему |
| `make docs` | Генерация документации |

## Расширенные возможности

### Кастомные ассерты

```c
/* Создание пользовательского ассерта */
bool test_assert_custom_condition(int value, const char *message) {
    bool passed = (value > 0 && value < 100);
    if (current_test_result) {
        add_assertion_to_result("CUSTOM_CONDITION", __FILE__, __LINE__, message, passed);
    }
    return passed;
}

#define TEST_ASSERT_CUSTOM(value, message) \
    TEST_ASSERT_BASE(test_assert_custom_condition(value, message), message)
```

### Плагины тестирования

```c
/* Пример плагина для тестирования */
test_plugin_t *create_performance_plugin(void) {
    test_plugin_t *plugin = malloc(sizeof(test_plugin_t));

    strcpy(plugin->name, "performance_monitor");
    plugin->process_result = monitor_test_performance;
    plugin->generate_report = generate_performance_report;

    return plugin;
}
```

### Анализ покрытия

```bash
# Сборка с анализом покрытия
make coverage

# Генерация отчета о покрытии
gcov build/*.gcno
```

## Интеграция с CI/CD

### Пример для GitHub Actions

```yaml
name: Tests
on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Build and test
      run: |
        cd test-framework
        make framework
        make test
        make coverage
```

## Лучшие практики

### Написание тестов

1. **Организация тестов**
   - Группируйте тесты по функциональности
   - Используйте описательные имена тестов
   - Следуйте соглашениям об именовании

2. **Ассерты**
   - Используйте подходящие типы ассертов
   - Добавляйте информативные сообщения об ошибках
   - Проверяйте граничные случаи

3. **Настройка и очистка**
   - Правильно настраивайте тестовые данные
   - Освобождайте ресурсы после тестов
   - Изолируйте тесты друг от друга

### Производительность

1. **Оптимизация**
   - Минимизируйте время выполнения тестов
   - Используйте параллельное выполнение при возможности
   - Мониторьте использование ресурсов

2. **Отладка**
   - Включайте подробное логирование при отладке
   - Используйте инструменты анализа памяти
   - Профилируйте критические тесты

## Расширение фреймворка

### Добавление новых типов ассертов

1. Реализуйте функцию ассерта в `test_asserts.c`
2. Добавьте соответствующий макрос в `test_macros.h`
3. Обновите документацию

### Создание кастомных плагинов

1. Реализуйте интерфейс `test_plugin_t`
2. Зарегистрируйте плагин в системе
3. Добавьте конфигурацию плагина

## Лицензия

Этот тестовый фреймворк является частью проекта SystemMonitor и распространяется под той же лицензией.

## Поддержка и развитие

Фреймворк активно развивается в соответствии с эволюцией архитектуры SystemMonitor v3.0. Для предложений по улучшению обращайтесь к документации проекта.