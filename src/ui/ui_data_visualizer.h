/**
 * UI Data Visualizer Module
 *
 * Отвечает за бизнес-логику отображения системных данных, процессов и метрик.
 * Изолирует логику визуализации данных от рендеринга и обработки ввода.
 */

#ifndef UI_DATA_VISUALIZER_H
#define UI_DATA_VISUALIZER_H

#include "ui_renderer.h"
#include "ui_input_handler.h"
#include <ncurses.h>

// Типы экранов для отображения
typedef enum {
    SCREEN_MAIN,           // Главный экран с системными метриками
    SCREEN_PROCESSES,      // Список процессов
    SCREEN_PROCESS_TREE,   // Дерево процессов
    SCREEN_NETWORK,        // Сетевая информация
    SCREEN_DIAGNOSTICS,    // Диагностика системы
    SCREEN_TESTING,        // Экран тестирования
    SCREEN_SETTINGS,       // Настройки
    SCREEN_WEATHER,        // Погода
    SCREEN_DOCKER,         // Docker контейнеры
    SCREEN_GIT_REPOS,      // Git репозитории
    SCREEN_DEV_ENV,        // Среда разработки
    SCREEN_LISTENING_PORTS // Занятые порты
} screen_type_t;

// Структуры определены в ui_renderer.h для избежания дублирования

/**
 * Инициализация модуля визуализации данных
 */
void ui_data_visualizer_init(void);

/**
 * Деинициализация модуля визуализации данных
 */
void ui_data_visualizer_cleanup(void);

/**
 * Отображение главного экрана с системными метриками
 * @param metrics - текущие системные метрики
 * @param history - история метрик для графиков
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_main_screen(const system_metrics_t *metrics,
                        const metrics_history_t *history, int colors_on);

/**
 * Отображение списка процессов
 * @param state - состояние ввода для навигации
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_processes_screen(input_state_t *state, int colors_on);

/**
 * Отображение дерева процессов
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_process_tree_screen(int colors_on);

/**
 * Отображение сетевой информации
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_network_screen(int colors_on);

/**
 * Отображение диагностического экрана
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_diagnostics_screen(int colors_on);

/**
 * Отображение экрана тестирования
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_testing_screen(int colors_on);

/**
 * Отображение экрана настроек
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_settings_screen(int colors_on);

/**
 * Отображение детальной информации о погоде
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_weather_details_screen(int colors_on);

/**
 * Отображение списка Docker контейнеров
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_docker_screen(int colors_on);

/**
 * Отображение списка Git репозиториев
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_git_repos_screen(int colors_on);

/**
 * Отображение информации о среде разработки
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_dev_environment_screen(int colors_on);

/**
 * Отображение списка занятых портов
 * @param colors_on - включить ли цветовое оформление
 */
void ui_show_listening_ports_screen(int colors_on);

/**
 * Обновление истории метрик
 * @param history - структура истории
 * @param cpu - текущее значение CPU
 * @param memory - текущее значение памяти
 */
void ui_update_metrics_history(metrics_history_t *history, float cpu, float memory);

/**
 * Получение текущих системных метрик
 * @param metrics - структура для заполнения метрик
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ui_get_system_metrics(system_metrics_t *metrics);

/**
 * Проверка на аномалии в метриках
 * @param current - текущие метрики
 * @param history - история метрик
 * @param sigma_threshold - порог сигма для обнаружения аномалий
 * @return битовый флаг аномалий (бит 0 - CPU, бит 1 - память)
 */
int ui_detect_anomalies(const system_metrics_t *current,
                       const metrics_history_t *history, float sigma_threshold);

/**
 * Проверка на критические значения метрик
 * @param metrics - текущие метрики
 * @param cpu_threshold - порог CPU для предупреждения
 * @param memory_threshold - порог памяти для предупреждения
 * @param temp_threshold - порог температуры для предупреждения
 * @return битовый флаг критичных состояний
 */
int ui_check_thresholds(const system_metrics_t *metrics,
                       float cpu_threshold, float memory_threshold, float temp_threshold);

/**
 * Сохранение снимка текущего состояния системы
 * @param filename - имя файла для сохранения
 * @param metrics - текущие метрики
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int ui_save_system_snapshot(const char *filename, const system_metrics_t *metrics);

/**
 * Получение строки форматированного времени работы
 * @param uptime_seconds - время работы в секундах
 * @param buffer - буфер для результата
 * @param buffer_size - размер буфера
 */
void ui_format_uptime(long uptime_seconds, char *buffer, size_t buffer_size);

/**
 * Получение строки с информацией о погоде
 * @param buffer - буфер для результата
 * @param buffer_size - размер буфера
 * @param city - город для запроса (может быть NULL)
 */
void ui_get_weather_info(char *buffer, size_t buffer_size, const char *city);

/**
 * Получение строки с информацией о сети
 * @param buffer - буфер для результата
 * @param buffer_size - размер буфера
 */
void ui_get_network_info(char *buffer, size_t buffer_size);

/**
 * Получение строки с информацией о диске
 * @param buffer - буфер для результата
 * @param buffer_size - размер буфера
 */
void ui_get_disk_info(char *buffer, size_t buffer_size);

#endif // UI_DATA_VISUALIZER_H