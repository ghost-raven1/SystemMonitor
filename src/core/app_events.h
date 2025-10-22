/**
 * @file app_events.h
 * @brief Определения событий приложения для модульной архитектуры
 *
 * Определяет типы событий, которые могут происходить в приложении,
 * и структуры данных для передачи информации о событиях между модулями.
 */

#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include <time.h>

// Типы событий приложения
typedef enum {
    // Системные события
    APP_EVENT_STARTUP,           // Приложение запускается
    APP_EVENT_SHUTDOWN,          // Приложение завершается
    APP_EVENT_TERMINAL_RESIZE,   // Изменен размер терминала

    // События модулей
    APP_EVENT_METRICS_UPDATED,   // Метрики системы обновлены
    APP_EVENT_SCREEN_CHANGED,    // Экран изменен
    APP_EVENT_ERROR_OCCURRED,    // Произошла ошибка
    APP_EVENT_MODULE_LOADED,     // Модуль загружен
    APP_EVENT_MODULE_UNLOADED,   // Модуль выгружен

    // События ввода-вывода
    APP_EVENT_KEY_PRESSED,       // Клавиша нажата
    APP_EVENT_SCREEN_REFRESH,    // Экран нуждается в обновлении
    APP_EVENT_DATA_RECEIVED,     // Получены новые данные

    // Специфичные события UI
    APP_EVENT_PROCESS_SELECTED,  // Процесс выбран в списке
    APP_EVENT_FILTER_APPLIED,    // Применен фильтр
    APP_EVENT_SNAPSHOT_SAVED,    // Снимок состояния сохранен

    // События мониторинга
    APP_EVENT_HIGH_CPU_USAGE,    // Высокая загрузка CPU
    APP_EVENT_HIGH_MEMORY_USAGE, // Высокая загрузка памяти
    APP_EVENT_HIGH_TEMPERATURE,  // Высокая температура

    // Максимальное значение для валидации
    APP_EVENT_MAX
} app_event_t;

// Структура данных события
typedef struct {
    app_event_t type;            // Тип события
    time_t timestamp;            // Время события
    int priority;                // Приоритет (0-10, где 10 - критично)
    char source[64];             // Источник события (имя модуля)
    char message[256];           // Сообщение события
    void *data;                  // Дополнительные данные (зависит от типа события)
    size_t data_size;            // Размер дополнительных данных
} app_event_data_t;

// Структура обработчика событий
typedef struct {
    app_event_t event_type;      // Тип события для обработки
    void (*handler)(const app_event_data_t *event_data); // Функция обработчика
    void *user_data;             // Пользовательские данные для обработчика
    int enabled;                 // Флаг включения обработчика
} event_handler_t;

// Максимальное количество обработчиков событий
#define MAX_EVENT_HANDLERS 64

// Структура менеджера событий
typedef struct {
    event_handler_t handlers[MAX_EVENT_HANDLERS];
    int handler_count;
    int enabled;
} event_manager_t;

// Функции для работы с событиями
int event_manager_init(void);
void event_manager_cleanup(void);
int register_event_handler(app_event_t event_type,
                          void (*handler)(const app_event_data_t *),
                          void *user_data);
int unregister_event_handler(app_event_t event_type,
                            void (*handler)(const app_event_data_t *));
int emit_event(app_event_t event_type, int priority, const char *source,
               const char *message, void *data, size_t data_size);
int emit_event_with_data(app_event_data_t *event_data);

// Вспомогательные функции для создания событий
app_event_data_t *create_event_data(app_event_t type, int priority,
                                   const char *source, const char *message);
void free_event_data(app_event_data_t *event_data);

// Предустановленные обработчики событий
void default_error_handler(const app_event_data_t *event_data);
void default_metrics_handler(const app_event_data_t *event_data);
void default_screen_handler(const app_event_data_t *event_data);

// Макросы для удобного создания событий
#define EMIT_ERROR_EVENT(source, message) \
    emit_event(APP_EVENT_ERROR_OCCURRED, 5, source, message, NULL, 0)

#define EMIT_METRICS_EVENT(source) \
    emit_event(APP_EVENT_METRICS_UPDATED, 3, source, "Metrics updated", NULL, 0)

#define EMIT_SCREEN_EVENT(source, message) \
    emit_event(APP_EVENT_SCREEN_CHANGED, 2, source, message, NULL, 0)

#endif // APP_EVENTS_H