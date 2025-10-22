/**
 * @file ui_diagnostics.h
 * @brief Заголовочный файл для расширенного экрана диагностики системы
 *
 * Предоставляет интерфейс для мониторинга состояния системы,
 * отображения производительности и диагностики ошибок в реальном времени.
 */

#ifndef UI_DIAGNOSTICS_H
#define UI_DIAGNOSTICS_H

#include <ncurses.h>
#include "../core/app_context.h"
#include "../modules/diagnostics.h"

// Определения структур данных диагностики
typedef struct {
    float cpu_usage;
    unsigned long memory_used;
    unsigned long memory_total;
    float disk_usage;
    float network_rx_rate;
    float network_tx_rate;
    int process_count;
    int thread_count;
    time_t timestamp;
} performance_data_t;

typedef struct {
    char module_name[64];
    int initialized;
    int status;
    char last_error[256];
    time_t last_check;
    float performance_impact;
} diagnostics_module_status_t;

typedef struct {
    performance_data_t current_perf;
    performance_data_t peak_perf;
    diagnostics_module_status_t modules[16];
    int module_count;
    char system_info[512];
    char error_log[1024];
    int error_count;
} diagnostics_info_t;

// Структура состояния экрана диагностики
typedef struct {
    int current_view;        // Текущий вид (0=обзор, 1=модули, 2=производительность, 3=логи, 4=проверки)
    int auto_refresh;        // Автообновление включено
    int refresh_interval;    // Интервал обновления в секундах
    int selected_module;     // Выбранный модуль для детального просмотра
    int graph_history_size;  // Размер истории для графиков
    performance_data_t perf_history[100]; // История производительности
    int perf_history_count;
} diagnostics_screen_state_t;

// Глобальное состояние экрана диагностики
extern diagnostics_screen_state_t g_diag_state;

// Основные функции диагностики UI
void ui_render_diagnostics_screen(void);
void ui_diagnostics_show_module_status(void);
void ui_diagnostics_show_performance_graphs(void);
void ui_diagnostics_show_error_log(void);
void ui_diagnostics_run_system_checks(void);

// Функции обновления данных
int ui_diagnostics_update_performance_data(performance_data_t *data);
int ui_diagnostics_update_module_status(diagnostics_module_status_t *modules, int *count);
int ui_diagnostics_update_error_log(char *log_buffer, int buffer_size);
int ui_diagnostics_collect_system_info(char *info_buffer, int buffer_size);

// Функции рендеринга компонентов
void ui_diagnostics_render_overview(WINDOW *win, const diagnostics_info_t *info);
void ui_diagnostics_render_module_list(WINDOW *win, const diagnostics_module_status_t *modules, int count, int selected);
void ui_diagnostics_render_performance_graphs(WINDOW *win, const performance_data_t *history, int history_count);
void ui_diagnostics_render_error_log(WINDOW *win, const char *error_log);
void ui_diagnostics_render_system_checks(WINDOW *win);

// Интерактивные функции
void ui_diagnostics_handle_input(int ch);
void ui_diagnostics_switch_view(int view);
void ui_diagnostics_toggle_auto_refresh(void);
void ui_diagnostics_run_network_diagnostics(void);
void ui_diagnostics_run_process_analysis(void);

// Интеграция с AppContext
int ui_diagnostics_sync_with_app_context(app_context_t *ctx);
int ui_diagnostics_get_system_metrics(void);

// Инициализация и очистка
int ui_diagnostics_init(void);
void ui_diagnostics_cleanup(void);

// UI функция тестирования функциональности модулей (только UI)
void ui_run_functionality_tests_ui(void);

#endif // UI_DIAGNOSTICS_H