/**
 * @file app_context.c
 * @brief Реализация контекста приложения для управления модулями SystemMonitor
 *
 * Централизованная структура для управления состоянием приложения,
 * координации модулей и предоставления глобального доступа к ресурсам.
 */

#include "core/app_context.h"
#include "utils/error_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Прототипы статических функций
static int app_context_init_modules(void);
static int app_context_cleanup_modules(void);

// Глобальный контекст приложения
app_context_t *g_app_context = NULL;
bool g_context_initialized = false;

// Статическая функция инициализации конфигурации по умолчанию
static void init_default_config(app_config_t *config) {
    if (!config) return;

    snprintf(config->config_file_path, sizeof(config->config_file_path), "config.ini");
    snprintf(config->data_directory, sizeof(config->data_directory), "./data");
    snprintf(config->log_directory, sizeof(config->log_directory), "./logs");
    snprintf(config->temp_directory, sizeof(config->temp_directory), "/tmp");

    config->max_log_files = 10;
    config->max_log_file_size_mb = 100;
    config->enable_networking = true;
    config->enable_notifications = true;
    config->update_interval_ms = 1000;
}

// Статическая функция инициализации глобального состояния по умолчанию
static void init_default_global_state(app_global_state_t *state) {
    if (!state) return;

    state->status = APP_STATUS_STOPPED;
    state->mode = APP_MODE_NORMAL;
    snprintf(state->version, sizeof(state->version), "2.0.0");
    snprintf(state->build_date, sizeof(state->build_date), __DATE__ " " __TIME__);
    state->main_pid = getpid();
    state->start_time = 0;
    state->last_update = 0;
    state->shutdown_requested = false;
    state->exit_code = 0;
}

// Статическая функция инициализации управления модулями по умолчанию
static void init_default_module_manager(module_manager_t *modules) {
    if (!modules) return;

    modules->system_monitor_initialized = false;
    modules->developer_tools_initialized = false;
    modules->diagnostics_initialized = false;
    modules->error_handler_initialized = false;
    modules->ui_initialized = false;
    modules->active_modules_count = 0;
    modules->failed_modules_count = 0;
    modules->last_module_check = 0;
}

// Статическая функция инициализации статистики производительности по умолчанию
static void init_default_performance_stats(performance_stats_t *stats) {
    if (!stats) return;

    stats->total_memory_usage_kb = 0;
    stats->average_cpu_usage = 0.0f;
    stats->active_threads_count = 0;
    stats->total_allocations = 0;
    stats->total_deallocations = 0;
    stats->measurement_start = time(NULL);
}

// Статическая функция инициализации обработчиков событий по умолчанию
static void init_default_event_handlers(event_handlers_t *handlers) {
    if (!handlers) return;

    handlers->on_status_changed = NULL;
    handlers->on_mode_changed = NULL;
    handlers->on_module_initialized = NULL;
    handlers->on_module_failed = NULL;
    handlers->on_shutdown = NULL;
}

// Функция инициализации контекста приложения
int app_context_init(const char *config_file) {
    if (g_context_initialized) {
        LOG_WARNING(ERR_INVALID_STATE, "Контекст приложения уже инициализирован", "app_context");
        return 0;
    }

    // Выделяем память для глобального контекста
    g_app_context = calloc(1, sizeof(app_context_t));
    if (!g_app_context) {
        LOG_ERROR(ERR_SYSTEM_MEMORY, "Не удалось выделить память для контекста приложения", "app_context");
        return -1;
    }

    // Инициализируем конфигурацию
    init_default_config(&g_app_context->config);
    if (config_file) {
        snprintf(g_app_context->config.config_file_path, sizeof(g_app_context->config.config_file_path), "%s", config_file);
    }

    // Инициализируем глобальное состояние
    init_default_global_state(&g_app_context->global_state);

    // Инициализируем управление модулями
    init_default_module_manager(&g_app_context->modules);

    // Инициализируем статистику производительности
    init_default_performance_stats(&g_app_context->performance);

    // Инициализируем обработчики событий
    init_default_event_handlers(&g_app_context->events);

    // Инициализируем мьютекс и условную переменную
    if (pthread_mutex_init(&g_app_context->mutex, NULL) != 0) {
        LOG_ERROR(ERR_SYSTEM_IO, "Не удалось инициализировать мьютекс контекста", "app_context");
        free(g_app_context);
        g_app_context = NULL;
        return -1;
    }

    if (pthread_cond_init(&g_app_context->status_changed, NULL) != 0) {
        LOG_ERROR(ERR_SYSTEM_IO, "Не удалось инициализировать условную переменную контекста", "app_context");
        pthread_mutex_destroy(&g_app_context->mutex);
        free(g_app_context);
        g_app_context = NULL;
        return -1;
    }

    g_context_initialized = true;

    // Загружаем конфигурацию из файла если указан
    if (config_file) {
        load_app_config(config_file);
    }

    LOG_INFO("Контекст приложения инициализирован", "app_context");
    return 0;
}

// Функция инициализации с пользовательской конфигурацией
int app_context_init_with_config(const app_config_t *config) {
    if (!config) {
        LOG_ERROR(ERR_INVALID_PARAMETER, "Конфигурация не может быть NULL", "app_context");
        return -1;
    }

    int result = app_context_init(config->config_file_path);
    if (result != 0) {
        return result;
    }

    // Применяем пользовательскую конфигурацию
    memcpy(&g_app_context->config, config, sizeof(app_config_t));

    return 0;
}

// Функция очистки контекста приложения
void app_context_cleanup(void) {
    if (!g_context_initialized) {
        return;
    }

    WITH_CONTEXT_LOCK({
        // Останавливаем приложение если оно запущено
        if (g_app_context->global_state.status == APP_STATUS_RUNNING) {
            app_context_stop();
        }

        // Освобождаем ресурсы синхронизации
        pthread_mutex_destroy(&g_app_context->mutex);
        pthread_cond_destroy(&g_app_context->status_changed);

        // Освобождаем память контекста
        free(g_app_context);
        g_app_context = NULL;
        g_context_initialized = false;
    });

    LOG_INFO("Контекст приложения очищен", "app_context");
}

// Функция запуска приложения
int app_context_start(void) {
    if (!g_context_initialized) {
        LOG_ERROR(ERR_INVALID_STATE, "Контекст приложения не инициализирован", "app_context");
        return -1;
    }

    WITH_CONTEXT_LOCK({
        if (g_app_context->global_state.status == APP_STATUS_RUNNING) {
            LOG_WARNING(ERR_INVALID_STATE, "Приложение уже запущено", "app_context");
            return 0;
        }

        app_status_t old_status = g_app_context->global_state.status;
        g_app_context->global_state.status = APP_STATUS_STARTING;
        g_app_context->global_state.start_time = time(NULL);

        // Вызываем обработчик изменения статуса
        if (g_app_context->events.on_status_changed) {
            g_app_context->events.on_status_changed(old_status, APP_STATUS_STARTING);
        }

        // Инициализируем модули
        if (app_context_init_modules() != 0) {
            LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать модули", "app_context");
            g_app_context->global_state.status = APP_STATUS_ERROR;
            return -1;
        }

        g_app_context->global_state.status = APP_STATUS_RUNNING;
        g_app_context->global_state.last_update = time(NULL);

        // Вызываем обработчик изменения статуса
        if (g_app_context->events.on_status_changed) {
            g_app_context->events.on_status_changed(APP_STATUS_STARTING, APP_STATUS_RUNNING);
        }
    });

    LOG_INFO("Приложение запущено", "app_context");
    return 0;
}

// Функция остановки приложения
int app_context_stop(void) {
    if (!g_context_initialized) {
        LOG_ERROR(ERR_INVALID_STATE, "Контекст приложения не инициализирован", "app_context");
        return -1;
    }

    WITH_CONTEXT_LOCK({
        if (g_app_context->global_state.status == APP_STATUS_STOPPED) {
            LOG_WARNING(ERR_INVALID_STATE, "Приложение уже остановлено", "app_context");
            return 0;
        }

        app_status_t old_status = g_app_context->global_state.status;
        g_app_context->global_state.status = APP_STATUS_STOPPING;

        // Вызываем обработчик изменения статуса
        if (g_app_context->events.on_status_changed) {
            g_app_context->events.on_status_changed(old_status, APP_STATUS_STOPPING);
        }

        // Очищаем модули
        app_context_cleanup_modules();

        g_app_context->global_state.status = APP_STATUS_STOPPED;
        g_app_context->global_state.shutdown_requested = true;

        // Вызываем обработчик изменения статуса
        if (g_app_context->events.on_status_changed) {
            g_app_context->events.on_status_changed(APP_STATUS_STOPPING, APP_STATUS_STOPPED);
        }

        // Вызываем обработчик завершения работы
        if (g_app_context->events.on_shutdown) {
            g_app_context->events.on_shutdown(g_app_context->global_state.exit_code);
        }
    });

    LOG_INFO("Приложение остановлено", "app_context");
    return 0;
}

// Получение глобального контекста
app_context_t *get_app_context(void) {
    return g_context_initialized ? g_app_context : NULL;
}

// Получение контекста только для чтения
const app_context_t *get_app_context_readonly(void) {
    return g_context_initialized ? g_app_context : NULL;
}

// Установка статуса приложения
int set_app_status(app_status_t status) {
    if (!g_context_initialized) {
        LOG_ERROR(ERR_INVALID_STATE, "Контекст приложения не инициализирован", "app_context");
        return -1;
    }

    WITH_CONTEXT_LOCK({
        app_status_t old_status = g_app_context->global_state.status;
        g_app_context->global_state.status = status;
        g_app_context->global_state.last_update = time(NULL);

        // Вызываем обработчик изменения статуса
        if (g_app_context->events.on_status_changed) {
            g_app_context->events.on_status_changed(old_status, status);
        }

        // Оповещаем ожидающие потоки
        pthread_cond_broadcast(&g_app_context->status_changed);
    });

    return 0;
}

// Получение текущего статуса приложения
app_status_t get_app_status(void) {
    if (!g_context_initialized || !g_app_context) {
        return APP_STATUS_STOPPED;
    }

    app_status_t status;
    WITH_CONTEXT_LOCK({
        status = g_app_context->global_state.status;
    });

    return status;
}

// Инициализация модулей через контекст
static int app_context_init_modules(void) {
    if (!g_context_initialized) {
        LOG_ERROR(ERR_INVALID_STATE, "Контекст приложения не инициализирован", "app_context");
        return -1;
    }

    WITH_CONTEXT_LOCK({
        int success_count = 0;
        int fail_count = 0;

        // Инициализируем модули в порядке зависимостей

        // 1. Error Handler (базовый модуль)
        if (!g_app_context->modules.error_handler_initialized) {
            error_config_t error_config = {0};
            error_config.enable_graceful_degradation = true;
            error_config.enable_auto_recovery = true;
            error_config.max_recovery_attempts = 3;
            error_config.recovery_cooldown_sec = 30;
            snprintf(error_config.log_file_path, sizeof(error_config.log_file_path), "%s/errors.log", g_app_context->config.log_directory);

            error_handler_init(&error_config);
            g_app_context->modules.error_handler_initialized = true;
            g_app_context->modules.active_modules_count++;
            success_count++;

            if (g_app_context->events.on_module_initialized) {
                g_app_context->events.on_module_initialized("error_handler");
            }
        }

        // 2. System Monitor
        if (!g_app_context->modules.system_monitor_initialized) {
            if (system_monitor_init() == 0) {
                if (system_monitor_start() == 0) {
                    g_app_context->modules.system_monitor_initialized = true;
                    g_app_context->modules.active_modules_count++;
                    success_count++;

                    if (g_app_context->events.on_module_initialized) {
                        g_app_context->events.on_module_initialized("system_monitor");
                    }
                } else {
                    LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось запустить system_monitor", "app_context");
                    fail_count++;
                }
            } else {
                LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать system_monitor", "app_context");
                fail_count++;
            }
        }

        // 3. Developer Tools
        if (!g_app_context->modules.developer_tools_initialized) {
            if (developer_tools_init(g_app_context->config.data_directory) == 0) {
                if (developer_tools_enable() == 0) {
                    g_app_context->modules.developer_tools_initialized = true;
                    g_app_context->modules.active_modules_count++;
                    success_count++;

                    if (g_app_context->events.on_module_initialized) {
                        g_app_context->events.on_module_initialized("developer_tools");
                    }
                } else {
                    LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось включить developer_tools", "app_context");
                    fail_count++;
                }
            } else {
                LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать developer_tools", "app_context");
                fail_count++;
            }
        }

        // 4. Diagnostics
        if (!g_app_context->modules.diagnostics_initialized) {
            if (diagnostics_init(g_app_context->config.data_directory) == 0) {
                if (diagnostics_enable() == 0) {
                    g_app_context->modules.diagnostics_initialized = true;
                    g_app_context->modules.active_modules_count++;
                    success_count++;

                    if (g_app_context->events.on_module_initialized) {
                        g_app_context->events.on_module_initialized("diagnostics");
                    }
                } else {
                    LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось включить diagnostics", "app_context");
                    fail_count++;
                }
            } else {
                LOG_ERROR(ERR_EXTERNAL_LIB, "Не удалось инициализировать diagnostics", "app_context");
                fail_count++;
            }
        }

        g_app_context->modules.failed_modules_count = fail_count;

        if (fail_count > 0) {
            LOG_WARNING(ERR_EXTERNAL_LIB, "Не удалось инициализировать некоторые модули", "app_context");
        }

        LOG_INFO("Инициализация модулей завершена", "app_context");
        return fail_count > 0 ? -1 : 0;
    });

    return 0;
}

// Очистка модулей через контекст
static int app_context_cleanup_modules(void) {
    if (!g_context_initialized) {
        LOG_ERROR(ERR_INVALID_STATE, "Контекст приложения не инициализирован", "app_context");
        return -1;
    }

    WITH_CONTEXT_LOCK({
        // Останавливаем и очищаем модули в обратном порядке зависимостей

        if (g_app_context->modules.diagnostics_initialized) {
            diagnostics_disable();
            diagnostics_cleanup();
            g_app_context->modules.diagnostics_initialized = false;
            g_app_context->modules.active_modules_count--;
        }

        if (g_app_context->modules.developer_tools_initialized) {
            developer_tools_disable();
            developer_tools_cleanup();
            g_app_context->modules.developer_tools_initialized = false;
            g_app_context->modules.active_modules_count--;
        }

        if (g_app_context->modules.system_monitor_initialized) {
            system_monitor_stop();
            system_monitor_cleanup();
            g_app_context->modules.system_monitor_initialized = false;
            g_app_context->modules.active_modules_count--;
        }

        if (g_app_context->modules.error_handler_initialized) {
            error_handler_cleanup();
            g_app_context->modules.error_handler_initialized = false;
            g_app_context->modules.active_modules_count--;
        }

        LOG_INFO("Очистка модулей завершена", "app_context");
    });

    return 0;
}

// Инициализация конкретного модуля
int initialize_module(const char *module_name) {
    if (!module_name) {
        LOG_ERROR(ERR_INVALID_PARAMETER, "Имя модуля не может быть NULL", "app_context");
        return -1;
    }

    if (!g_context_initialized) {
        LOG_ERROR(ERR_INVALID_STATE, "Контекст приложения не инициализирован", "app_context");
        return -1;
    }

    WITH_CONTEXT_LOCK({
        if (strcmp(module_name, "system_monitor") == 0) {
            if (!g_app_context->modules.system_monitor_initialized) {
                if (system_monitor_init() == 0 && system_monitor_start() == 0) {
                    g_app_context->modules.system_monitor_initialized = true;
                    g_app_context->modules.active_modules_count++;

                    if (g_app_context->events.on_module_initialized) {
                        g_app_context->events.on_module_initialized(module_name);
                    }

                    return 0;
                } else {
                    if (g_app_context->events.on_module_failed) {
                        g_app_context->events.on_module_failed(module_name, "Инициализация завершилась неудачей");
                    }
                    return -1;
                }
            }
        }
        else if (strcmp(module_name, "developer_tools") == 0) {
            if (!g_app_context->modules.developer_tools_initialized) {
                if (developer_tools_init(g_app_context->config.data_directory) == 0 &&
                    developer_tools_enable() == 0) {
                    g_app_context->modules.developer_tools_initialized = true;
                    g_app_context->modules.active_modules_count++;

                    if (g_app_context->events.on_module_initialized) {
                        g_app_context->events.on_module_initialized(module_name);
                    }

                    return 0;
                } else {
                    if (g_app_context->events.on_module_failed) {
                        g_app_context->events.on_module_failed(module_name, "Инициализация завершилась неудачей");
                    }
                    return -1;
                }
            }
        }
        else if (strcmp(module_name, "diagnostics") == 0) {
            if (!g_app_context->modules.diagnostics_initialized) {
                if (diagnostics_init(g_app_context->config.data_directory) == 0 &&
                    diagnostics_enable() == 0) {
                    g_app_context->modules.diagnostics_initialized = true;
                    g_app_context->modules.active_modules_count++;

                    if (g_app_context->events.on_module_initialized) {
                        g_app_context->events.on_module_initialized(module_name);
                    }

                    return 0;
                } else {
                    if (g_app_context->events.on_module_failed) {
                        g_app_context->events.on_module_failed(module_name, "Инициализация завершилась неудачей");
                    }
                    return -1;
                }
            }
        }
        else if (strcmp(module_name, "error_handler") == 0) {
            if (!g_app_context->modules.error_handler_initialized) {
                error_config_t error_config = {0};
                error_config.enable_graceful_degradation = true;
                error_config.enable_auto_recovery = true;
                error_config.max_recovery_attempts = 3;
                error_config.recovery_cooldown_sec = 30;
                snprintf(error_config.log_file_path, sizeof(error_config.log_file_path), "%s/errors.log", g_app_context->config.log_directory);

                error_handler_init(&error_config);
                g_app_context->modules.error_handler_initialized = true;
                g_app_context->modules.active_modules_count++;

                if (g_app_context->events.on_module_initialized) {
                    g_app_context->events.on_module_initialized(module_name);
                }

                return 0;
            }
        }
        else {
            LOG_ERROR(ERR_INVALID_PARAMETER, "Неизвестный модуль", module_name);
            return -1;
        }
    });

    return 0;
}

// Проверка инициализации модуля
bool is_module_initialized(const char *module_name) {
    if (!module_name || !g_context_initialized || !g_app_context) {
        return false;
    }

    bool initialized = false;

    WITH_CONTEXT_LOCK({
        if (strcmp(module_name, "system_monitor") == 0) {
            initialized = g_app_context->modules.system_monitor_initialized;
        }
        else if (strcmp(module_name, "developer_tools") == 0) {
            initialized = g_app_context->modules.developer_tools_initialized;
        }
        else if (strcmp(module_name, "diagnostics") == 0) {
            initialized = g_app_context->modules.diagnostics_initialized;
        }
        else if (strcmp(module_name, "error_handler") == 0) {
            initialized = g_app_context->modules.error_handler_initialized;
        }
        else if (strcmp(module_name, "ui") == 0) {
            initialized = g_app_context->modules.ui_initialized;
        }
    });

    return initialized;
}

// Заглушки для остальных функций (будут реализованы по мере необходимости)
int shutdown_module(const char *module_name) { return 0; }
int get_initialized_modules(char *buffer, int buffer_size) { return 0; }
int update_app_context(void) { return 0; }
int refresh_performance_stats(void) { return 0; }
int register_status_change_handler(void (*handler)(app_status_t, app_status_t)) { return 0; }
int register_mode_change_handler(void (*handler)(app_mode_t, app_mode_t)) { return 0; }
int register_module_handler(void (*init_handler)(const char *), void (*fail_handler)(const char *, const char *)) { return 0; }
int register_shutdown_handler(void (*handler)(int)) { return 0; }
int load_app_config(const char *config_file) { return 0; }
int save_app_config(const char *config_file) { return 0; }
int update_app_config(const app_config_t *new_config) { return 0; }
const app_config_t *get_app_config(void) { return g_context_initialized ? &g_app_context->config : NULL; }
int execute_with_context_lock(int (*function)(void *), void *arg) { return 0; }
bool is_app_context_valid(void) { return g_context_initialized && g_app_context != NULL; }
const char *get_app_status_string(app_status_t status) { return "unknown"; }
const char *get_app_mode_string(app_mode_t mode) { return "unknown"; }
int set_app_mode(app_mode_t mode) { return 0; }
app_mode_t get_app_mode(void) { return APP_MODE_NORMAL; }
int app_context_pause(void) { return 0; }
int app_context_resume(void) { return 0; }
int app_context_restart(void) { return 0; }

// Проверка активности главного цикла приложения
int app_context_is_running(void) {
    if (!g_context_initialized || !g_app_context) {
        return 0; // Не запущено
    }

    app_status_t status;
    WITH_CONTEXT_LOCK({
        status = g_app_context->global_state.status;
    });

    // Возвращаем 1 если приложение в состоянии RUNNING или STARTING
    return (status == APP_STATUS_RUNNING || status == APP_STATUS_STARTING) ? 1 : 0;
}
