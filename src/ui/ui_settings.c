// ui_settings.c - Экран настроек приложения SystemMonitor
#include "ui_renderer.h"
#include "ui_theme.h"
#include "utils/logging.h"
#include "utils/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Структура для описания элементов меню настроек
typedef struct {
    const char *name;           // Название настройки
    const char *description;    // Описание
    int type;                   // Тип: 0=bool, 1=int, 2=enum, 3=float
    void *value_ptr;           // Указатель на значение в app_settings_t
    const char **enum_values;  // Значения для enum типа
    int min_val;               // Минимальное значение (для int/float)
    int max_val;               // Максимальное значение (для int/float)
    float step;                // Шаг изменения (для float)
} settings_menu_item_t;

// Названия тем
static const char *theme_names[] = {
    "BTOP_DARK",
    "NEON",
    "MATRIX",
    "LIGHT",
    NULL
};

// Названия метрик
static const char *metric_names[] = {
    "CPU", "RAM", "DISK", "NET", "TEMP", NULL
};

// Элементы меню настроек
static settings_menu_item_t settings_menu[] = {
    // Основные настройки
    {"Интервал обновления", "Интервал обновления в секундах (1-60)", 1,
     NULL, NULL, 1, 60, 1},
    {"Тема интерфейса", "Цветовая схема интерфейса", 2,
     NULL, theme_names, 0, 0, 0},
    {"Отображаемые метрики", "Какие метрики показывать", 2,
     NULL, metric_names, 0, 0, 0},

    // Настройки сети
    {"Мониторинг сети", "Включить сбор сетевой статистики", 0,
     NULL, NULL, 0, 0, 0},

    // Настройки безопасности
    {"Логирование", "Включить логирование событий", 0,
     NULL, NULL, 0, 0, 0},
    {"Валидация путей", "Проверять безопасность путей", 0,
     NULL, NULL, 0, 0, 0},

    // Настройки модулей
    {"Мониторинг батареи", "Отслеживать состояние батареи", 0,
     NULL, NULL, 0, 0, 0},
    {"Мониторинг GPU", "Отслеживать видеокарту", 0,
     NULL, NULL, 0, 0, 0},
    {"Мониторинг USB", "Отслеживать USB устройства", 0,
     NULL, NULL, 0, 0, 0},
    {"SMART мониторинг", "Мониторинг состояния дисков", 0,
     NULL, NULL, 0, 0, 0},
    {"Мониторинг SMC", "Температуры через SMC", 0,
     NULL, NULL, 0, 0, 0},

    // Пороговые значения CPU
    {"CPU предупреждение", "Порог предупреждения CPU (%)", 3,
     NULL, NULL, 50, 95, 1.0},
    {"CPU критично", "Порог критического значения CPU (%)", 3,
     NULL, NULL, 80, 100, 1.0},

    // Пороговые значения памяти
    {"Память предупреждение", "Порог предупреждения памяти (%)", 3,
     NULL, NULL, 60, 95, 1.0},
    {"Память критично", "Порог критического значения памяти (%)", 3,
     NULL, NULL, 80, 100, 1.0},

    // Пороговые значения температуры
    {"Темп. предупреждение", "Порог предупреждения температуры (°C)", 3,
     NULL, NULL, 50, 90, 1.0},
    {"Темп. критично", "Порог критического значения температуры (°C)", 3,
     NULL, NULL, 70, 100, 1.0},

    // Интерфейс
    {"Использовать цвета", "Включить цветовое оформление", 0,
     NULL, NULL, 0, 0, 0},
    {"Unicode символы", "Использовать Unicode для рамок", 0,
     NULL, NULL, 0, 0, 0},
    {"Показывать иконки", "Отображать emoji иконки", 0,
     NULL, NULL, 0, 0, 0},

    // Процессы
    {"Процессов на странице", "Количество процессов на странице", 1,
     NULL, NULL, 5, 50, 1},
    {"Системные процессы", "Показывать системные процессы", 0,
     NULL, NULL, 0, 0, 0},
    {"Процессы др. пользователей", "Показывать процессы других пользователей", 0,
     NULL, NULL, 0, 0, 0},

    // Детекция аномалий
    {"Чувствительность аномалий", "Чувствительность обнаружения аномалий", 3,
     NULL, NULL, 1.0, 5.0, 0.1},

    // Экспорт данных
    {"Экспорт Prometheus", "Экспорт метрик в Prometheus", 0,
     NULL, NULL, 0, 0, 0},
    {"Экспорт Grafana", "Экспорт метрик в Grafana", 0,
     NULL, NULL, 0, 0, 0},
    {"Экспорт InfluxDB", "Экспорт метрик в InfluxDB", 0,
     NULL, NULL, 0, 0, 0},

    // Уведомления
    {"Звуковые уведомления", "Звуковые уведомления", 0,
     NULL, NULL, 0, 0, 0},
    {"Системные уведомления", "Системные уведомления", 0,
     NULL, NULL, 0, 0, 0},

    {NULL, NULL, 0, NULL, NULL, 0, 0, 0} // Конец массива
};

// Получение количества элементов меню
int ui_settings_get_menu_count(void) {
    int count = 0;
    while (settings_menu[count].name != NULL) {
        count++;
    }
    return count;
}

// Получение описания элемента меню
void ui_settings_get_menu_item(int index, char *buffer, size_t buffer_size,
                              const char *current_value, const app_settings_t *settings) {
    if (index < 0 || index >= ui_settings_get_menu_count()) {
        snprintf(buffer, buffer_size, "Неверный индекс");
        return;
    }

    settings_menu_item_t *item = &settings_menu[index];

    if (item->type == 0) {
        // Булево значение
        snprintf(buffer, buffer_size, "%s: %s",
                item->description, current_value);
    } else if (item->type == 1) {
        // Целое значение
        snprintf(buffer, buffer_size, "%s: %s",
                item->description, current_value);
    } else if (item->type == 2) {
        // Перечисление
        snprintf(buffer, buffer_size, "%s: %s",
                item->description, current_value);
    } else if (item->type == 3) {
        // Дробное значение
        snprintf(buffer, buffer_size, "%s: %s",
                item->description, current_value);
    }
}

// Обновление значения настройки
const char *ui_settings_update_value(int index, int action, app_settings_t *settings) {
    static char value_str[64];

    if (index < 0 || index >= ui_settings_get_menu_count()) {
        return "Ошибка";
    }

    settings_menu_item_t *item = &settings_menu[index];

    switch (item->type) {
        case 0: // Булево значение
            switch (index) {
                case 3: // Мониторинг сети
                    settings->network_monitoring = !settings->network_monitoring;
                    return settings->network_monitoring ? "ВКЛ" : "ВЫКЛ";
                case 4: // Логирование
                    settings->enable_logging = !settings->enable_logging;
                    return settings->enable_logging ? "ВКЛ" : "ВЫКЛ";
                case 5: // Валидация путей
                    settings->validate_paths = !settings->validate_paths;
                    return settings->validate_paths ? "ВКЛ" : "ВЫКЛ";
                case 6: // Мониторинг батареи
                    settings->battery_monitoring = !settings->battery_monitoring;
                    return settings->battery_monitoring ? "ВКЛ" : "ВЫКЛ";
                case 7: // Мониторинг GPU
                    settings->gpu_monitoring = !settings->gpu_monitoring;
                    return settings->gpu_monitoring ? "ВКЛ" : "ВЫКЛ";
                case 8: // Мониторинг USB
                    settings->usb_monitoring = !settings->usb_monitoring;
                    return settings->usb_monitoring ? "ВКЛ" : "ВЫКЛ";
                case 9: // SMART мониторинг
                    settings->smart_monitoring = !settings->smart_monitoring;
                    return settings->smart_monitoring ? "ВКЛ" : "ВЫКЛ";
                case 10: // Мониторинг SMC
                    settings->smc_temp_monitoring = !settings->smc_temp_monitoring;
                    return settings->smc_temp_monitoring ? "ВКЛ" : "ВЫКЛ";
                case 18: // Использовать цвета
                    settings->use_colors = !settings->use_colors;
                    return settings->use_colors ? "ВКЛ" : "ВЫКЛ";
                case 19: // Unicode символы
                    settings->use_unicode = !settings->use_unicode;
                    return settings->use_unicode ? "ВКЛ" : "ВЫКЛ";
                case 20: // Показывать иконки
                    settings->show_icons = !settings->show_icons;
                    return settings->show_icons ? "ВКЛ" : "ВЫКЛ";
                case 22: // Системные процессы
                    settings->show_system_processes = !settings->show_system_processes;
                    return settings->show_system_processes ? "ВКЛ" : "ВЫКЛ";
                case 23: // Процессы других пользователей
                    settings->show_other_users = !settings->show_other_users;
                    return settings->show_other_users ? "ВКЛ" : "ВЫКЛ";
                case 26: // Экспорт Prometheus
                    settings->prometheus_export = !settings->prometheus_export;
                    return settings->prometheus_export ? "ВКЛ" : "ВЫКЛ";
                case 27: // Экспорт Grafana
                    settings->grafana_export = !settings->grafana_export;
                    return settings->grafana_export ? "ВКЛ" : "ВЫКЛ";
                case 28: // Экспорт InfluxDB
                    settings->influxdb_export = !settings->influxdb_export;
                    return settings->influxdb_export ? "ВКЛ" : "ВЫКЛ";
                case 30: // Звуковые уведомления
                    settings->sound_notifications = !settings->sound_notifications;
                    return settings->sound_notifications ? "ВКЛ" : "ВЫКЛ";
                case 31: // Системные уведомления
                    settings->system_notifications = !settings->system_notifications;
                    return settings->system_notifications ? "ВКЛ" : "ВЫКЛ";
                default:
                    return "Неизвестно";
            }

        case 1: // Целое значение
            switch (index) {
                case 0: // Интервал обновления
                    if (action == 0 && settings->update_interval > item->min_val) {
                        settings->update_interval--;
                    } else if (action == 1 && settings->update_interval < item->max_val) {
                        settings->update_interval++;
                    }
                    snprintf(value_str, sizeof(value_str), "%d сек", settings->update_interval);
                    return value_str;
                case 21: // Процессов на странице
                    if (action == 0 && settings->processes_per_page > item->min_val) {
                        settings->processes_per_page--;
                    } else if (action == 1 && settings->processes_per_page < item->max_val) {
                        settings->processes_per_page++;
                    }
                    snprintf(value_str, sizeof(value_str), "%d", settings->processes_per_page);
                    return value_str;
                default:
                    return "Неизвестно";
            }

        case 2: // Перечисление
            switch (index) {
                case 1: // Тема интерфейса
                    if (action == 0 && settings->theme > 0) {
                        settings->theme--;
                    } else if (action == 1 && settings->theme < THEME_LIGHT) {
                        settings->theme++;
                    }
                    return theme_names[settings->theme];
                case 2: // Метрики
                    if (action == 0 && settings->enabled_metrics > METRIC_CPU) {
                        settings->enabled_metrics--;
                    } else if (action == 1 && settings->enabled_metrics < (METRIC_TEMP << 1) - 1) {
                        settings->enabled_metrics++;
                    }
                    snprintf(value_str, sizeof(value_str), "%d", settings->enabled_metrics);
                    return value_str;
                default:
                    return "Неизвестно";
            }

        case 3: // Дробное значение
            switch (index) {
                case 11: // CPU предупреждение
                    if (action == 0 && settings->cpu_warning_threshold > item->min_val) {
                        settings->cpu_warning_threshold -= item->step;
                    } else if (action == 1 && settings->cpu_warning_threshold < item->max_val) {
                        settings->cpu_warning_threshold += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f%%", settings->cpu_warning_threshold);
                    return value_str;
                case 12: // CPU критично
                    if (action == 0 && settings->cpu_critical_threshold > item->min_val) {
                        settings->cpu_critical_threshold -= item->step;
                    } else if (action == 1 && settings->cpu_critical_threshold < item->max_val) {
                        settings->cpu_critical_threshold += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f%%", settings->cpu_critical_threshold);
                    return value_str;
                case 13: // Память предупреждение
                    if (action == 0 && settings->memory_warning_threshold > item->min_val) {
                        settings->memory_warning_threshold -= item->step;
                    } else if (action == 1 && settings->memory_warning_threshold < item->max_val) {
                        settings->memory_warning_threshold += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f%%", settings->memory_warning_threshold);
                    return value_str;
                case 14: // Память критично
                    if (action == 0 && settings->memory_critical_threshold > item->min_val) {
                        settings->memory_critical_threshold -= item->step;
                    } else if (action == 1 && settings->memory_critical_threshold < item->max_val) {
                        settings->memory_critical_threshold += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f%%", settings->memory_critical_threshold);
                    return value_str;
                case 15: // Температура предупреждение
                    if (action == 0 && settings->temp_warning_threshold > item->min_val) {
                        settings->temp_warning_threshold -= item->step;
                    } else if (action == 1 && settings->temp_warning_threshold < item->max_val) {
                        settings->temp_warning_threshold += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f°C", settings->temp_warning_threshold);
                    return value_str;
                case 16: // Температура критично
                    if (action == 0 && settings->temp_critical_threshold > item->min_val) {
                        settings->temp_critical_threshold -= item->step;
                    } else if (action == 1 && settings->temp_critical_threshold < item->max_val) {
                        settings->temp_critical_threshold += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f°C", settings->temp_critical_threshold);
                    return value_str;
                case 25: // Чувствительность аномалий
                    if (action == 0 && settings->anomaly_sigma > item->min_val) {
                        settings->anomaly_sigma -= item->step;
                    } else if (action == 1 && settings->anomaly_sigma < item->max_val) {
                        settings->anomaly_sigma += item->step;
                    }
                    snprintf(value_str, sizeof(value_str), "%.1f", settings->anomaly_sigma);
                    return value_str;
                default:
                    return "Неизвестно";
            }

        default:
            return "Неизвестный тип";
    }
}

// Отрисовка экрана настроек
void ui_render_settings_screen(int rows, int cols, const app_settings_t *settings,
                              int selected_item, int colors_on) {
    int menu_count = ui_settings_get_menu_count();
    int start_y = 3;
    int max_items = rows - 6; // Оставляем место для заголовка и кнопок

    // Заголовок
    ui_draw_modern_header("НАСТРОЙКИ СИСТЕМЫ МОНИТОРИНГА");

    // Вычисляем видимый диапазон элементов
    int start_item = 0;
    int end_item = menu_count - 1;

    if (menu_count > max_items) {
        int half = max_items / 2;
        if (selected_item < half) {
            start_item = 0;
            end_item = max_items - 1;
        } else if (selected_item >= menu_count - half) {
            start_item = menu_count - max_items;
            end_item = menu_count - 1;
        } else {
            start_item = selected_item - half;
            end_item = selected_item + half;
        }
    }

    // Отрисовываем элементы меню
    for (int i = start_item; i <= end_item && i < menu_count; i++) {
        int y = start_y + (i - start_item);
        int color_pair = (i == selected_item) ? COLOR_PAIR_CYAN : COLOR_PAIR_WHITE;

        if (colors_on) {
            attron(COLOR_PAIR(color_pair));
        }

        // Получаем текущее значение настройки
        const char *current_value = ui_settings_update_value(i, -1, (app_settings_t *)settings);
        char menu_item[256];

        if (i == selected_item) {
            snprintf(menu_item, sizeof(menu_item), "▶ %s", settings_menu[i].name);
        } else {
            snprintf(menu_item, sizeof(menu_item), "  %s", settings_menu[i].name);
        }

        // Обрезаем строку если она слишком длинная
        if (strlen(menu_item) > cols - 20) {
            menu_item[cols - 23] = '\0';
            strcat(menu_item, "...");
        }

        mvprintw(y, 2, "%-*s : %s", cols - 30, menu_item, current_value);

        if (colors_on) {
            attroff(COLOR_PAIR(color_pair));
        }
    }

    // Кнопки управления
    int button_y = rows - 3;
    if (colors_on) {
        attron(COLOR_PAIR(COLOR_PAIR_GREEN));
    }
    mvprintw(button_y, 2, "↑↓ - Выбор настройки");
    if (colors_on) {
        attroff(COLOR_PAIR(COLOR_PAIR_GREEN));
    }

    if (colors_on) {
        attron(COLOR_PAIR(COLOR_PAIR_YELLOW));
    }
    mvprintw(button_y, cols/2 - 10, "←→ - Изменение значения");
    if (colors_on) {
        attroff(COLOR_PAIR(COLOR_PAIR_YELLOW));
    }

    if (colors_on) {
        attron(COLOR_PAIR(COLOR_PAIR_MAGENTA));
    }
    mvprintw(button_y + 1, 2, "S - Сохранить | C - Отмена | R - По умолчанию");
    if (colors_on) {
        attroff(COLOR_PAIR(COLOR_PAIR_MAGENTA));
    }
}

// Инициализация настроек по умолчанию
void ui_settings_init_defaults(app_settings_t *settings) {
    if (!settings) return;

    settings->update_interval = DEFAULT_UPDATE_INTERVAL;
    settings->theme = DEFAULT_THEME;
    settings->enabled_metrics = DEFAULT_METRICS;

    settings->network_monitoring = true;
    settings->enable_logging = true;
    settings->validate_paths = true;

    settings->battery_monitoring = true;
    settings->gpu_monitoring = true;
    settings->usb_monitoring = true;
    settings->smart_monitoring = true;
    settings->smc_temp_monitoring = true;

    settings->cpu_warning_threshold = DEFAULT_CPU_WARNING;
    settings->cpu_critical_threshold = DEFAULT_CPU_CRITICAL;
    settings->memory_warning_threshold = DEFAULT_MEMORY_WARNING;
    settings->memory_critical_threshold = DEFAULT_MEMORY_CRITICAL;
    settings->temp_warning_threshold = DEFAULT_TEMP_WARNING;
    settings->temp_critical_threshold = DEFAULT_TEMP_CRITICAL;

    settings->use_colors = true;
    settings->use_unicode = true;
    settings->show_icons = true;
    settings->transparency = 0;

    settings->processes_per_page = 15;
    settings->show_system_processes = true;
    settings->show_other_users = true;
    settings->process_auto_refresh = 2;

    settings->anomaly_sigma = 2.0f;

    settings->prometheus_export = false;
    settings->grafana_export = false;
    settings->influxdb_export = false;

    settings->sound_notifications = false;
    settings->system_notifications = false;
    settings->email_notifications = false;
    settings->telegram_notifications = false;
}

// Валидация настроек
int ui_settings_validate(const app_settings_t *settings) {
    if (!settings) return -1;

    if (settings->update_interval < 1 || settings->update_interval > 60) {
        log_error("Неверный интервал обновления");
        return -1;
    }

    if (settings->cpu_warning_threshold >= settings->cpu_critical_threshold) {
        log_error("Порог предупреждения CPU должен быть меньше критического");
        return -1;
    }

    if (settings->memory_warning_threshold >= settings->memory_critical_threshold) {
        log_error("Порог предупреждения памяти должен быть меньше критического");
        return -1;
    }

    if (settings->temp_warning_threshold >= settings->temp_critical_threshold) {
        log_error("Порог предупреждения температуры должен быть меньше критического");
        return -1;
    }

    if (settings->processes_per_page < 5 || settings->processes_per_page > 50) {
        log_error("Неверное количество процессов на странице");
        return -1;
    }

    return 0;
}

// Загрузка настроек из файла конфигурации
int ui_settings_load_config(app_settings_t *settings, const char *config_file) {
    if (!settings || !config_file) return -1;

    // Инициализируем значения по умолчанию
    ui_settings_init_defaults(settings);

    // Загружаем конфигурацию
    if (load_config(config_file) != 0) {
        log_error("Не удалось загрузить конфигурацию");
        return -1;
    }

    // Читаем переменные окружения и применяем настройки
    const char *value;

    // Интервал обновления
    value = getenv("update_interval");
    if (value) {
        int interval = atoi(value);
        if (interval >= 1 && interval <= 60) {
            settings->update_interval = interval;
        }
    }

    // Синхронизируем с AppContext если доступен
    app_context_t *ctx = get_app_context();
    if (ctx && ctx->config.update_interval_ms > 0) {
        settings->update_interval = ctx->config.update_interval_ms / 1000; // Конвертируем из мс в секунды
    }

    // Тема
    value = getenv("theme");
    if (value) {
        if (strcmp(value, "BTOP_DARK") == 0) settings->theme = THEME_BTOP_DARK;
        else if (strcmp(value, "NEON") == 0) settings->theme = THEME_NEON;
        else if (strcmp(value, "MATRIX") == 0) settings->theme = THEME_MATRIX;
        else if (strcmp(value, "LIGHT") == 0) settings->theme = THEME_LIGHT;
    }

    // Метрики
    value = getenv("enabled_metrics");
    if (value) {
        settings->enabled_metrics = (unsigned int)atoi(value);
    }

    // Булевы настройки
    settings->network_monitoring = (getenv("network_monitoring") &&
                                   strcmp(getenv("network_monitoring"), "1") == 0);
    settings->enable_logging = (getenv("enable_logging") &&
                               strcmp(getenv("enable_logging"), "1") == 0);
    settings->validate_paths = (getenv("validate_paths") &&
                               strcmp(getenv("validate_paths"), "1") == 0);

    // Модули
    settings->battery_monitoring = (getenv("battery") &&
                                   strcmp(getenv("battery"), "1") == 0);
    settings->gpu_monitoring = (getenv("gpu") &&
                              strcmp(getenv("gpu"), "1") == 0);
    settings->usb_monitoring = (getenv("usb") &&
                              strcmp(getenv("usb"), "1") == 0);
    settings->smart_monitoring = (getenv("smart") &&
                                 strcmp(getenv("smart"), "1") == 0);
    settings->smc_temp_monitoring = (getenv("smc_temps") &&
                                    strcmp(getenv("smc_temps"), "1") == 0);

    // Пороги CPU
    value = getenv("cpu_warning");
    if (value) settings->cpu_warning_threshold = atof(value);
    value = getenv("cpu_critical");
    if (value) settings->cpu_critical_threshold = atof(value);

    // Пороги памяти
    value = getenv("memory_warning");
    if (value) settings->memory_warning_threshold = atof(value);
    value = getenv("memory_critical");
    if (value) settings->memory_critical_threshold = atof(value);

    // Пороги температуры
    value = getenv("temperature_warning");
    if (value) settings->temp_warning_threshold = atof(value);
    value = getenv("temperature_critical");
    if (value) settings->temp_critical_threshold = atof(value);

    // Интерфейс
    settings->use_colors = (getenv("use_colors") &&
                           strcmp(getenv("use_colors"), "1") == 0);
    settings->use_unicode = (getenv("use_unicode") &&
                            strcmp(getenv("use_unicode"), "1") == 0);
    settings->show_icons = (getenv("show_icons") &&
                           strcmp(getenv("show_icons"), "1") == 0);

    // Процессы
    value = getenv("processes_per_page");
    if (value) {
        int ppp = atoi(value);
        if (ppp >= 5 && ppp <= 50) {
            settings->processes_per_page = ppp;
        }
    }

    settings->show_system_processes = (getenv("show_system_processes") &&
                                      strcmp(getenv("show_system_processes"), "1") == 0);
    settings->show_other_users = (getenv("show_other_users") &&
                                 strcmp(getenv("show_other_users"), "1") == 0);

    // Аномалии
    value = getenv("anomaly_sigma");
    if (value) settings->anomaly_sigma = atof(value);

    return 0;
}

// Сохранение настроек в файл конфигурации
int ui_settings_save_config(const app_settings_t *settings, const char *config_file) {
    if (!settings || !config_file) return -1;

    // Валидация перед сохранением
    if (ui_settings_validate(settings) != 0) {
        log_error("Настройки невалидны, сохранение отменено");
        return -1;
    }

    // Создаем резервную копию переменных окружения
    char env_backup[4096] = {0};

    // Сохраняем настройки как переменные окружения
    snprintf(env_backup, sizeof(env_backup),
             "update_interval=%d\n"
             "theme=%s\n"
             "enabled_metrics=%u\n"
             "network_monitoring=%d\n"
             "enable_logging=%d\n"
             "validate_paths=%d\n"
             "battery=%d\n"
             "gpu=%d\n"
             "usb=%d\n"
             "smart=%d\n"
             "smc_temps=%d\n"
             "cpu_warning=%.1f\n"
             "cpu_critical=%.1f\n"
             "memory_warning=%.1f\n"
             "memory_critical=%.1f\n"
             "temperature_warning=%.1f\n"
             "temperature_critical=%.1f\n"
             "use_colors=%d\n"
             "use_unicode=%d\n"
             "show_icons=%d\n"
             "processes_per_page=%d\n"
             "show_system_processes=%d\n"
             "show_other_users=%d\n"
             "anomaly_sigma=%.1f\n",
             settings->update_interval,
             theme_names[settings->theme],
             settings->enabled_metrics,
             settings->network_monitoring ? 1 : 0,
             settings->enable_logging ? 1 : 0,
             settings->validate_paths ? 1 : 0,
             settings->battery_monitoring ? 1 : 0,
             settings->gpu_monitoring ? 1 : 0,
             settings->usb_monitoring ? 1 : 0,
             settings->smart_monitoring ? 1 : 0,
             settings->smc_temp_monitoring ? 1 : 0,
             settings->cpu_warning_threshold,
             settings->cpu_critical_threshold,
             settings->memory_warning_threshold,
             settings->memory_critical_threshold,
             settings->temp_warning_threshold,
             settings->temp_critical_threshold,
             settings->use_colors ? 1 : 0,
             settings->use_unicode ? 1 : 0,
             settings->show_icons ? 1 : 0,
             settings->processes_per_page,
             settings->show_system_processes ? 1 : 0,
             settings->show_other_users ? 1 : 0,
             settings->anomaly_sigma);

    // Сохраняем настройки в файл
    if (save_config(config_file) != 0) {
        log_error("Не удалось сохранить конфигурацию");
        return -1;
    }

    // Применяем настройки к AppContext если доступен
    app_context_t *ctx = get_app_context();
    if (ctx) {
        // Обновляем интервал обновления в AppContext
        ctx->config.update_interval_ms = settings->update_interval * 1000; // Конвертируем в мс

        // Обновляем другие настройки если нужно
        ctx->config.enable_networking = settings->network_monitoring;
        ctx->config.enable_notifications = settings->sound_notifications;

        // Синхронизируем тему с глобальным состоянием UI
        ui_theme_set_theme(settings->theme);

        log_info("Настройки применены к AppContext");
    }

    log_info("Настройки успешно сохранены");
    return 0;
}