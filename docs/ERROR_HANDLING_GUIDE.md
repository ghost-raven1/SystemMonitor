# Руководство по системе обработки ошибок SystemMonitor

## Обзор

Система обработки ошибок SystemMonitor предоставляет централизованную архитектуру для классификации, логирования и восстановления от ошибок. Система обеспечивает graceful degradation и автоматическое восстановление модулей.

## Архитектура

### Основные компоненты

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Error Handler │    │   Module Manager │    │   Recovery      │
│   (Центральный) │◄──►│   (Состояния)    │◄──►│   (Автоматич.)  │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Логирование   │    │   Graceful       │    │   Мониторинг    │
│   (Файлы/Консоль)│    │   Degradation    │    │   (Метрики)     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## Классификация ошибок

### Коды ошибок

| Диапазон | Категория | Описание |
|----------|-----------|----------|
| 1000-1099 | Системные | Ошибки памяти, I/O, разрешения |
| 2000-2099 | Мониторинг | Ошибки модулей сбора данных |
| 3000-3099 | Интерфейс | Ошибки UI и взаимодействия |
| 4000-4099 | Конфигурация | Ошибки загрузки/парсинга конфига |
| 5000-5099 | Сеть | Ошибки сетевых операций |
| 6000-6099 | Зависимости | Ошибки внешних библиотек |
| 7000-7099 | Логика | Ошибки состояния приложения |

### Уровни серьезности

| Уровень | Значение | Поведение |
|---------|----------|-----------|
| DEBUG | Отладка | Только логирование |
| INFO | Информация | Логирование + уведомление |
| WARNING | Предупреждение | Логирование + возможное отключение |
| ERROR | Ошибка | Логирование + отключение модуля |
| CRITICAL | Критическая | Немедленное отключение + восстановление |
| FATAL | Фатальная | Завершение приложения |

## Использование

### Инициализация

```c
#include "error_handler.h"

int main(void) {
    // Настройка системы обработки ошибок
    error_config_t config = {
        .enable_graceful_degradation = true,
        .enable_auto_recovery = true,
        .max_recovery_attempts = 3,
        .recovery_cooldown_sec = 30,
        .log_file_path = "error.log",
        .custom_handler = NULL
    };

    error_handler_init(&config);

    // ... остальной код приложения

    error_handler_cleanup();
    return 0;
}
```

### Регистрация ошибок

```c
// Простая регистрация ошибки
LOG_ERROR(ERR_BATTERY_INIT, "Failed to initialize battery", "Battery module");

// С ручным созданием структуры ошибки
app_error_t error = create_error(
    ERR_BATTERY_READ,
    SEVERITY_WARNING,
    "Low battery level detected",
    "Battery percentage below threshold",
    "battery"
);
register_error(&error);
```

### Работа с модулями

```c
// Проверка состояния модуля
if (!is_module_enabled("battery")) {
    printf("Battery module is disabled\n");
    return -1; // Graceful degradation
}

// Безопасное выполнение операций модуля
CHECK_MODULE_ENABLED("battery");

// Обработка ошибок модуля
if (battery_operation_failed) {
    HANDLE_MODULE_FAILURE("battery", "Hardware not available");
}
```

## Graceful Degradation

### Автоматическое отключение модулей

Система автоматически отключает модули при критических ошибках:

```c
// Пример: модуль батареи отключается при аппаратных ошибках
if (hardware_battery_unavailable) {
    LOG_CRITICAL(ERR_BATTERY_INIT,
                "Battery hardware not available",
                "Hardware fault");
    // Модуль автоматически отключается системой
}
```

### Проверка доступности функций

```c
void display_battery_info(void) {
    if (!is_module_enabled("battery")) {
        printf("Battery information not available\n");
        return;
    }

    // ... отображение информации о батарее
}
```

## Механизмы восстановления

### Автоматическое восстановление

```c
// Попытка восстановления модуля
if (should_attempt_recovery("battery")) {
    attempt_module_recovery("battery");
    if (is_module_enabled("battery")) {
        printf("Battery module recovered successfully\n");
    }
}

// Ручной сброс счетчиков ошибок
reset_module_failures("battery");
```

### Кастомная логика восстановления

```c
// Определение кастомного обработчика
void custom_recovery_handler(const app_error_t *error) {
    if (error->code == ERR_BATTERY_READ) {
        // Специфичная логика восстановления батареи
        system("kextload -b com.apple.driver.AppleSmartBatteryManager");
    }
}

error_config_t config = {
    // ... другие настройки
    .custom_handler = custom_recovery_handler
};
```

## Мониторинг и диагностика

### Получение статистики модулей

```c
void print_module_status(void) {
    const char *modules[] = {"battery", "cpu", "memory", "network", NULL};

    for (int i = 0; modules[i] != NULL; i++) {
        printf("Module: %s\n", modules[i]);
        printf("  Enabled: %s\n", is_module_enabled(modules[i]) ? "Yes" : "No");
        printf("  Failed: %s\n", is_module_failed(modules[i]) ? "Yes" : "No");
        printf("  Failure count: %d\n", get_module_failure_count(modules[i]));
        printf("\n");
    }
}
```

### Анализ логов ошибок

Логи ошибок записываются в формате:
```
[2024-01-15 14:30:25] ERROR:battery:IOPSCopyPowerSourcesInfo [Battery hardware not available] Failed to get power sources info
```

## Интеграция с существующими модулями

### Миграция от старой системы

**До:**
```c
if (!power_info) {
    log_error("Failed to get power sources info");
    return -1;
}
```

**После:**
```c
if (!power_info) {
    LOG_ERROR(ERR_BATTERY_INIT, "Failed to get power sources info", "IOPSCopyPowerSourcesInfo");
    return -1;
}
```

### Рекомендации по использованию

1. **Используйте соответствующие коды ошибок** для каждого модуля
2. **Предоставляйте контекст** для облегчения диагностики
3. **Проверяйте состояние модулей** перед выполнением операций
4. **Используйте graceful degradation** для не критичных функций
5. **Настраивайте параметры восстановления** в соответствии с требованиями

## Конфигурация

### Параметры конфигурации

| Параметр | Значение по умолчанию | Описание |
|----------|----------------------|----------|
| enable_graceful_degradation | true | Включить автоматическое отключение модулей |
| enable_auto_recovery | true | Включить автоматическое восстановление |
| max_recovery_attempts | 3 | Максимальное количество попыток восстановления |
| recovery_cooldown_sec | 30 | Пауза между попытками восстановления (сек) |
| log_file_path | "error.log" | Путь к файлу логов |

## Лучшие практики

### Обработка ошибок в модулях

```c
int battery_init(void) {
    battery_info_t *info = get_battery_info();
    if (!info) {
        LOG_ERROR(ERR_BATTERY_INIT, "Battery initialization failed", "Hardware unavailable");
        return -1;
    }

    if (!is_module_enabled("battery")) {
        LOG_WARNING(ERR_BATTERY_READ, "Battery module disabled", "Previous failures");
        return -1;
    }

    return 0;
}
```

### Восстановление после сбоев

```c
void battery_monitor(void) {
    while (monitoring_active) {
        if (should_attempt_recovery("battery")) {
            LOG_INFO("Attempting battery recovery", "Scheduled recovery");
            attempt_module_recovery("battery");
        }

        if (is_module_enabled("battery")) {
            // Выполнение мониторинга батареи
            battery_info_t info;
            if (get_battery_info(&info) == 0) {
                // Обработка успешного получения данных
            }
        }

        sleep(60); // Проверка каждую минуту
    }
}
```

## Тестирование

### Модульные тесты

```bash
gcc test_error_handler.c src/error_handler.c -o test_error_handler -lpthread
./test_error_handler
```

### Тесты включают:
- Создание и регистрацию ошибок
- Управление состоянием модулей
- Graceful degradation
- Механизмы восстановления
- Кастомные обработчики
- Макросы логирования

## Заключение

Централизованная система обработки ошибок обеспечивает надежность и стабильность SystemMonitor за счет:

- **Классификации ошибок** для быстрой диагностики
- **Graceful degradation** для сохранения функциональности
- **Автоматического восстановления** для минимизации простоев
- **Централизованного логирования** для эффективного мониторинга
- **Гибкой конфигурации** для адаптации под различные сценарии использования

Система спроектирована для минимального влияния на производительность при максимальной надежности работы приложения.