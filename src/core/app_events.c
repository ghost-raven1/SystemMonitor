/**
 * @file app_events.c
 * @brief Реализация менеджера событий приложения
 *
 * Обеспечивает централизованную обработку событий между модулями,
 * включая регистрацию обработчиков, генерацию и доставку событий.
 */

#include "core/app_events.h"
#include "utils/logging.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Статический менеджер событий
static event_manager_t g_event_manager = {0};

// Инициализация менеджера событий
int event_manager_init(void) {
    memset(&g_event_manager, 0, sizeof(event_manager_t));
    g_event_manager.enabled = 1;
    return 0;
}

// Очистка менеджера событий
void event_manager_cleanup(void) {
    memset(&g_event_manager, 0, sizeof(event_manager_t));
}

// Регистрация обработчика событий
int register_event_handler(app_event_t event_type,
                          void (*handler)(const app_event_data_t *),
                          void *user_data) {
    if (!handler || !g_event_manager.enabled) return -1;

    if (g_event_manager.handler_count >= MAX_EVENT_HANDLERS) {
        log_error("Превышено максимальное количество обработчиков событий");
        return -1;
    }

    // Проверяем, не зарегистрирован ли уже такой обработчик
    for (int i = 0; i < g_event_manager.handler_count; i++) {
        if (g_event_manager.handlers[i].event_type == event_type &&
            g_event_manager.handlers[i].handler == handler) {
            return 0; // Уже зарегистрирован
        }
    }

    // Регистрируем новый обработчик
    event_handler_t *eh = &g_event_manager.handlers[g_event_manager.handler_count++];
    eh->event_type = event_type;
    eh->handler = handler;
    eh->user_data = user_data;
    eh->enabled = 1;

    return 0;
}

// Отмена регистрации обработчика событий
int unregister_event_handler(app_event_t event_type,
                            void (*handler)(const app_event_data_t *)) {
    if (!handler || !g_event_manager.enabled) return -1;

    for (int i = 0; i < g_event_manager.handler_count; i++) {
        if (g_event_manager.handlers[i].event_type == event_type &&
            g_event_manager.handlers[i].handler == handler) {
            // Удаляем обработчик (перемещаем последний на его место)
            if (i < g_event_manager.handler_count - 1) {
                memcpy(&g_event_manager.handlers[i],
                       &g_event_manager.handlers[g_event_manager.handler_count - 1],
                       sizeof(event_handler_t));
            }
            g_event_manager.handler_count--;
            return 0;
        }
    }

    return -1; // Обработчик не найден
}

// Генерация и отправка события
int emit_event(app_event_t event_type, int priority, const char *source,
               const char *message, void *data, size_t data_size) {
    if (!g_event_manager.enabled) return -1;

    app_event_data_t event_data;
    event_data.type = event_type;
    event_data.timestamp = time(NULL);
    event_data.priority = priority;
    snprintf(event_data.source, sizeof(event_data.source), "%s", source ? source : "unknown");
    snprintf(event_data.message, sizeof(event_data.message), "%s", message ? message : "");

    if (data && data_size > 0) {
        event_data.data = malloc(data_size);
        if (!event_data.data) return -1;
        memcpy(event_data.data, data, data_size);
        event_data.data_size = data_size;
    } else {
        event_data.data = NULL;
        event_data.data_size = 0;
    }

    int result = emit_event_with_data(&event_data);

    if (event_data.data) {
        free(event_data.data);
    }

    return result;
}

// Отправка события с готовыми данными
int emit_event_with_data(app_event_data_t *event_data) {
    if (!event_data || !g_event_manager.enabled) return -1;

    // Отправляем событие всем зарегистрированным обработчикам
    int handled_count = 0;
    for (int i = 0; i < g_event_manager.handler_count; i++) {
        event_handler_t *eh = &g_event_manager.handlers[i];
        if (eh->enabled && eh->handler &&
            (eh->event_type == event_data->type || eh->event_type == APP_EVENT_MAX)) {
            eh->handler(event_data);
            handled_count++;
        }
    }

    return handled_count;
}

// Создание данных события
app_event_data_t *create_event_data(app_event_t type, int priority,
                                   const char *source, const char *message) {
    app_event_data_t *event_data = malloc(sizeof(app_event_data_t));
    if (!event_data) return NULL;

    event_data->type = type;
    event_data->timestamp = time(NULL);
    event_data->priority = priority;
    snprintf(event_data->source, sizeof(event_data->source), "%s", source ? source : "unknown");
    snprintf(event_data->message, sizeof(event_data->message), "%s", message ? message : "");
    event_data->data = NULL;
    event_data->data_size = 0;

    return event_data;
}

// Освобождение данных события
void free_event_data(app_event_data_t *event_data) {
    if (!event_data) return;

    if (event_data->data) {
        free(event_data->data);
    }
    free(event_data);
}

// Предустановленные обработчики событий

// Обработчик ошибок по умолчанию
void default_error_handler(const app_event_data_t *event_data) {
    if (!event_data) return;

    log_error("Событие ошибки");
    fprintf(stderr, "ERROR [%s]: %s\n", event_data->source, event_data->message);
}

// Обработчик обновления метрик по умолчанию
void default_metrics_handler(const app_event_data_t *event_data) {
    if (!event_data) return;

    if (getenv("SYSMON_DEBUG")) {
        fprintf(stderr, "DBG: Metrics updated from %s: %s\n",
                event_data->source, event_data->message);
    }
}

// Обработчик событий экрана по умолчанию
void default_screen_handler(const app_event_data_t *event_data) {
    if (!event_data) return;

    if (getenv("SYSMON_DEBUG")) {
        fprintf(stderr, "DBG: Screen event from %s: %s\n",
                event_data->source, event_data->message);
    }
}