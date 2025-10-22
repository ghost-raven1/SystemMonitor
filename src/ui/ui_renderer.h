/**
 * UI Renderer Module
 *
 * Отвечает за отрисовку графики, текста и визуальных элементов интерфейса.
 * Содержит чистые функции рендеринга без бизнес-логики.
 */

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <ncurses.h>
#include "../core/app_context.h"  // Для app_context_t

// Предварительные объявления типов для избежания циклических зависимостей
typedef struct {
    float cpu_usage;
    float memory_usage;
    long uptime_seconds;
    char cpu_freq[64];
    int core_count;
    float core_temperatures[16];
    int temp_count;
    float memory_temp;
} system_metrics_t;

typedef struct {
    float cpu_history[60];
    float memory_history[60];
    int history_index;
} metrics_history_t;

typedef struct {
    int current_screen;
    void *input_state;
    system_metrics_t system_metrics;
    metrics_history_t metrics_history;
    int colors_enabled;
    int running;
    int needs_redraw;
    app_context_t *app_context;
} ui_state_t;

// Color pair constants
#define COLOR_PAIR_RED     1
#define COLOR_PAIR_GREEN   2
#define COLOR_PAIR_YELLOW  3
#define COLOR_PAIR_CYAN    4
#define COLOR_PAIR_WHITE   5
#define COLOR_PAIR_MAGENTA 6

// Типы тем интерфейса
typedef enum {
    THEME_BTOP_DARK,
    THEME_NEON,
    THEME_MATRIX,
    THEME_LIGHT
} theme_type_t;

// Типы отображаемых метрик
typedef enum {
    METRIC_CPU = (1 << 0),
    METRIC_RAM = (1 << 1),
    METRIC_DISK = (1 << 2),
    METRIC_NET = (1 << 3),
    METRIC_TEMP = (1 << 4)
} metric_type_t;

// Структура для настроек приложения
typedef struct {
    // Основные настройки
    int update_interval;              // Интервал обновления (1-60 сек)
    theme_type_t theme;               // Тема интерфейса
    unsigned int enabled_metrics;     // Включенные метрики (битовая маска)

    // Настройки сети
    bool network_monitoring;          // Включить мониторинг сети

    // Настройки безопасности
    bool enable_logging;              // Включить логирование
    bool validate_paths;              // Валидация путей

    // Настройки модулей
    bool battery_monitoring;          // Мониторинг батареи
    bool gpu_monitoring;              // Мониторинг GPU
    bool usb_monitoring;              // Мониторинг USB
    bool smart_monitoring;            // SMART мониторинг дисков
    bool smc_temp_monitoring;         // Мониторинг температур SMC

    // Пороговые значения
    float cpu_warning_threshold;      // Порог предупреждения CPU (%)
    float cpu_critical_threshold;     // Порог критичного значения CPU (%)
    float memory_warning_threshold;   // Порог предупреждения памяти (%)
    float memory_critical_threshold;  // Порог критичного значения памяти (%)
    float temp_warning_threshold;     // Порог предупреждения температуры (°C)
    float temp_critical_threshold;    // Порог критичного значения температуры (°C)

    // Настройки интерфейса
    bool use_colors;                  // Использовать цвета
    bool use_unicode;                 // Использовать Unicode символы
    bool show_icons;                  // Показывать иконки
    int transparency;                 // Прозрачность (0-100)

    // Настройки процессов
    int processes_per_page;           // Процессов на странице
    bool show_system_processes;       // Показывать системные процессы
    bool show_other_users;            // Показывать процессы других пользователей
    int process_auto_refresh;         // Автообновление процессов (сек)

    // Детекция аномалий
    float anomaly_sigma;              // Чувствительность аномалий
    int anomaly_window_size;          // Размер окна анализа
    int min_anomaly_samples;          // Мин. замеров для детекции

    // Экспорт данных
    bool prometheus_export;           // Экспорт в Prometheus
    bool grafana_export;              // Экспорт в Grafana
    bool influxdb_export;             // Экспорт в InfluxDB

    // Уведомления
    bool sound_notifications;         // Звуковые уведомления
    bool system_notifications;        // Системные уведомления
    bool email_notifications;         // Email уведомления
    bool telegram_notifications;      // Telegram уведомления
} app_settings_t;

// Значения по умолчанию для настроек
#define DEFAULT_UPDATE_INTERVAL      2     // секунды
#define DEFAULT_THEME               THEME_BTOP_DARK
#define DEFAULT_METRICS            (METRIC_CPU | METRIC_RAM | METRIC_DISK | METRIC_NET | METRIC_TEMP)
#define DEFAULT_CPU_WARNING        85.0f
#define DEFAULT_CPU_CRITICAL       95.0f
#define DEFAULT_MEMORY_WARNING     90.0f
#define DEFAULT_MEMORY_CRITICAL    95.0f
#define DEFAULT_TEMP_WARNING       75.0f
#define DEFAULT_TEMP_CRITICAL      85.0f

/**
 * Инициализация модуля рендеринга
 * Должна вызываться после инициализации ncurses
 */
void ui_renderer_init(void);

/**
 * Деинициализация модуля рендеринга
 */
void ui_renderer_cleanup(void);

/**
 * Вывод текста с обрезкой по ширине терминала
 * @param y - строка для вывода
 * @param x - колонка для вывода
 * @param fmt - формат строки (как printf)
 * @param ... - аргументы формата
 */
void ui_printw_clip(int y, int x, const char *fmt, ...);

/**
 * Вывод цветного текста с обрезкой по ширине терминала
 * @param y - строка для вывода
 * @param x - колонка для вывода
 * @param color_pair - пара цветов ncurses
 * @param fmt - формат строки (как printf)
 * @param ... - аргументы формата
 */
void ui_printw_clip_color(int y, int x, int color_pair, const char *fmt, ...);

/**
 * Отрисовка горизонтального разделителя
 * @param y - строка для отрисовки
 * @param x - начальная колонка
 */
void ui_draw_hsep(int y, int x);

/**
 * Отрисовка полосы загрузки
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param width - ширина полосы
 * @param percent - процент заполнения (0-100)
 * @param colors_on - включить ли цветовое оформление
 */
void ui_render_bar(int y, int x, int width, float percent, int colors_on);

/**
 * Выбор цвета по процентному значению
 * @param percent - процент (0-100)
 * @param warn_threshold - порог предупреждения
 * @param crit_threshold - порог критичного значения
 * @return пара цветов ncurses
 */
int ui_choose_color_by_percent(float percent, float warn_threshold, float crit_threshold);


/**
 * Отрисовка заголовка в стиле btop
 * @param title - текст заголовка
 */
void ui_draw_modern_header(const char *title);


/**
 * Отрисовка текстовой метки с иконкой
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param icon - иконка (может быть NULL)
 * @param label - текст метки
 * @param value - значение для отображения
 * @param color_pair - пара цветов (0 - без цвета)
 */
void ui_draw_labeled_value(int y, int x, const char *icon, const char *label,
                          const char *value, int color_pair);

/**
 * Отрисовка статусной строки с индикаторами модулей
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param module_status - строка со статусом модулей
 */
void ui_draw_status_bar(int y, int x, const char *module_status);

/**
 * Отрисовка рамки для графика истории
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param title - заголовок графика
 */
void ui_draw_history_frame(int y, int x, const char *title);

/**
 * Отрисовка верхней панели в стиле btop
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param width - ширина панели
 * @param system_info - информация о системе
 */
void ui_draw_btop_top_panel(int y, int x, int width, const char *system_info);

/**
 * Отрисовка боковой панели в стиле btop
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param height - высота панели
 * @param width - ширина панели
 * @param metrics - системные метрики
 */
void ui_draw_btop_side_panel(int y, int x, int height, int width, const system_metrics_t *metrics);

/**
 * Отрисовка нижней панели в стиле btop
 * @param y - строка для отрисовки
 * @param x - колонка для отрисовки
 * @param width - ширина панели
 * @param stats - статистика
 */
void ui_draw_btop_bottom_panel(int y, int x, int width, const char *stats);

/**
   * Отрисовка полного layout в стиле btop
   * @param rows - количество строк
   * @param cols - количество колонок
   * @param state - состояние UI
   */
 void ui_draw_btop_layout(int rows, int cols, ui_state_t *state);

/**
   * Основной рендерер главного экрана в стиле btop
   * @param rows - количество строк терминала
   * @param cols - количество колонок терминала
   * @param state - состояние UI с метриками системы
   */
void ui_render_main_screen(int rows, int cols, ui_state_t *state);

/**
   * Рендерер верхней панели с системной информацией
   * @param state - состояние UI с метриками системы
   */
void ui_render_top_bar(ui_state_t *state);

/**
   * Рендерер боковой панели с топ процессами
   * @param state - состояние UI с метриками системы
   */
void ui_render_side_panel(ui_state_t *state);

/**
   * Рендерер нижней панели со статистикой сети и диска
   * @param state - состояние UI с метриками системы
   */
void ui_render_bottom_bar(ui_state_t *state);

/**
   * Получение информации о топ процессах для боковой панели
   * @param buffer - буфер для результата
   * @param buffer_size - размер буфера
   */
void ui_get_top_processes_info(char *buffer, size_t buffer_size);

/**
   * Получение системной информации для верхней панели
   * @param buffer - буфер для результата
   * @param buffer_size - размер буфера
   * @param metrics - системные метрики
   */
void ui_get_system_info_string(char *buffer, size_t buffer_size, const system_metrics_t *metrics);

/**
    * Получение статистики сети и диска для нижней панели
    * @param buffer - буфер для результата
    * @param buffer_size - размер буфера
    */
void ui_get_network_disk_stats(char *buffer, size_t buffer_size);

/**
    * Форматирование времени работы системы
    * @param uptime_seconds - время работы в секундах
    * @param buffer - буфер для результата
    * @param buffer_size - размер буфера
    */
void ui_format_uptime(long uptime_seconds, char *buffer, size_t buffer_size);

/**
   * Отрисовка экрана тестирования системы
   * @param rows - количество строк терминала
   * @param cols - количество колонок терминала
   * @param progress - прогресс инициализации (0-100)
   * @param module_status - строка со статусом модулей
   * @param colors_on - включить ли цветовое оформление
   */
 void ui_render_testing_screen(int rows, int cols, float progress,
                              const char *module_status, int colors_on);

/**
   * Отрисовка экрана настроек
   * @param rows - количество строк терминала
   * @param cols - количество колонок терминала
   * @param settings - настройки приложения
   * @param selected_item - выбранный элемент меню
   * @param colors_on - включить ли цветовое оформление
   */
void ui_render_settings_screen(int rows, int cols, const app_settings_t *settings,
                              int selected_item, int colors_on);

/**
   * Получение количества элементов в меню настроек
   * @return количество элементов меню
   */
int ui_settings_get_menu_count(void);

/**
   * Получение описания элемента меню настроек
   * @param index - индекс элемента
   * @param buffer - буфер для описания
   * @param buffer_size - размер буфера
   * @param current_value - текущее значение настройки
   * @param settings - настройки приложения
   */
void ui_settings_get_menu_item(int index, char *buffer, size_t buffer_size,
                              const char *current_value, const app_settings_t *settings);

/**
   * Обновление настройки по индексу меню
   * @param index - индекс элемента меню
   * @param action - действие (0 - уменьшить, 1 - увеличить)
   * @param settings - настройки приложения (обновляются)
   * @return новая строка значения настройки
   */
const char *ui_settings_update_value(int index, int action, app_settings_t *settings);

/**
   * Сохранение настроек в файл конфигурации
   * @param settings - настройки для сохранения
   * @param config_file - путь к файлу конфигурации
   * @return 0 при успехе, -1 при ошибке
   */
int ui_settings_save_config(const app_settings_t *settings, const char *config_file);

/**
   * Загрузка настроек из файла конфигурации
   * @param settings - настройки для загрузки
   * @param config_file - путь к файлу конфигурации
   * @return 0 при успехе, -1 при ошибке
   */
int ui_settings_load_config(app_settings_t *settings, const char *config_file);

/**
   * Инициализация настроек значениями по умолчанию
   * @param settings - настройки для инициализации
   */
void ui_settings_init_defaults(app_settings_t *settings);

/**
   * Валидация настроек
   * @param settings - настройки для проверки
   * @return 0 при успехе, -1 при ошибке
   */
int ui_settings_validate(const app_settings_t *settings);

#endif // UI_RENDERER_H