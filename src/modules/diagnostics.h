/**
 * @file diagnostics.h
 * @brief Интерфейс модуля диагностики системы
 *
 * Предоставляет функции для диагностики аппаратного обеспечения,
 * анализа производительности и выявления проблем системы.
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>
#include <time.h>

// Типы диагностических тестов
typedef enum {
    DIAG_TEST_QUICK = 0,       // Быстрая диагностика
    DIAG_TEST_FULL = 1,        // Полная диагностика
    DIAG_TEST_STRESS = 2,      // Стресс-тестирование
    DIAG_TEST_CUSTOM = 3       // Пользовательская диагностика
} diagnostic_test_type_t;

// Компоненты для диагностики
typedef enum {
    DIAG_COMPONENT_CPU = 1,    // Процессор
    DIAG_COMPONENT_MEMORY = 2, // Память
    DIAG_COMPONENT_DISK = 4,   // Диск
    DIAG_COMPONENT_NETWORK = 8,// Сеть
    DIAG_COMPONENT_BATTERY = 16, // Батарея
    DIAG_COMPONENT_TEMPERATURE = 32, // Температура
    DIAG_COMPONENT_ALL = 63    // Все компоненты
} diagnostic_component_t;

// Статус диагностического теста
typedef enum {
    DIAG_STATUS_NOT_RUN = 0,   // Не запущен
    DIAG_STATUS_RUNNING = 1,   // Выполняется
    DIAG_STATUS_PASSED = 2,    // Пройден успешно
    DIAG_STATUS_FAILED = 3,    // Провалился
    DIAG_STATUS_WARNING = 4,   // Предупреждение
    DIAG_STATUS_ERROR = 5      // Ошибка
} diagnostic_status_t;

// Структура для результата диагностики компонента
typedef struct {
    diagnostic_component_t component;
    diagnostic_status_t status;
    char test_name[64];        // Название теста
    char description[256];     // Описание результатов
    float performance_score;   // Оценка производительности (0-100)
    char warnings[512];        // Предупреждения
    char errors[512];          // Ошибки
    time_t test_duration_ms;   // Длительность теста в мс
    time_t timestamp;          // Временная метка теста
} diagnostic_result_t;

// Структура для CPU диагностики
typedef struct {
    int core_count;            // Количество ядер
    float max_frequency_mhz;   // Максимальная частота
    float temperature_celsius; // Температура
    float load_average[3];     // Средняя загрузка (1, 5, 15 мин)
    bool supports_avx;         // Поддержка AVX инструкций
    bool supports_sse;         // Поддержка SSE инструкций
} cpu_diagnostic_info_t;

// Структура для диагностики памяти
typedef struct {
    unsigned long total_memory_mb;  // Общий объем памяти
    float usage_percent;       // Процент использования
    unsigned long available_mb;     // Доступная память
    int memory_sticks_count;   // Количество модулей памяти
    char memory_type[32];      // Тип памяти (DDR3, DDR4 и т.д.)
    unsigned long speed_mhz;   // Скорость памяти
} memory_diagnostic_info_t;

// Структура для диагностики диска
typedef struct {
    char device_name[32];      // Имя устройства
    unsigned long total_space_gb;   // Общий объем в GB
    unsigned long free_space_gb;    // Свободное пространство в GB
    float read_speed_mbps;     // Скорость чтения
    float write_speed_mbps;    // Скорость записи
    int temperature_celsius;   // Температура диска
    int health_percentage;     // Состояние здоровья (0-100)
} disk_diagnostic_info_t;

// Структура для сетевой диагностики
typedef struct {
    char interface_name[32];   // Имя интерфейса
    unsigned long rx_bytes;    // Получено байт
    unsigned long tx_bytes;    // Отправлено байт
    float rx_speed_mbps;       // Скорость приема
    float tx_speed_mbps;       // Скорость передачи
    int signal_strength;       // Сила сигнала WiFi (0-100)
    char ip_address[16];       // IP адрес
    char mac_address[18];      // MAC адрес
} network_diagnostic_info_t;

// Структура состояния диагностического модуля
typedef struct {
    bool diagnostics_enabled;
    diagnostic_test_type_t current_test_type;
    diagnostic_component_t components_to_test;
    int test_timeout_ms;       // Таймаут теста в миллисекундах
    int max_concurrent_tests;  // Максимум одновременных тестов
    bool generate_reports;     // Генерировать отчеты
    char report_directory[256]; // Директория для отчетов
} diagnostics_state_t;

// Инициализация и деинициализация модуля
int diagnostics_init(const char *report_dir);
void diagnostics_cleanup(void);
int diagnostics_enable(void);
int diagnostics_disable(void);

// Запуск диагностических тестов
int run_diagnostics(diagnostic_test_type_t test_type,
                   diagnostic_component_t components);
int run_cpu_diagnostics(cpu_diagnostic_info_t *result);
int run_memory_diagnostics(memory_diagnostic_info_t *result);
int run_disk_diagnostics(disk_diagnostic_info_t *result);
int run_network_diagnostics(network_diagnostic_info_t *result);

// Получение результатов диагностики
int get_diagnostic_results(diagnostic_result_t **results, int *count);
int get_latest_diagnostic_result(diagnostic_component_t component,
                                diagnostic_result_t *result);
diagnostic_status_t get_overall_system_status(void);

// Анализ производительности
float calculate_system_performance_score(void);
int identify_performance_bottlenecks(char *buffer, int buffer_size);
int generate_performance_report(const char *filename);

// Мониторинг здоровья системы
int start_health_monitoring(int interval_seconds);
int stop_health_monitoring(void);
bool is_system_healthy(void);

// Коллбэки для диагностики
typedef void (*diagnostic_complete_callback_t)(const diagnostic_result_t *result);
typedef void (*health_status_callback_t)(diagnostic_status_t status);

// Регистрация коллбэков
int register_diagnostic_callback(diagnostic_complete_callback_t callback);
int register_health_status_callback(health_status_callback_t callback);

// Конфигурация диагностики
int set_test_timeout(int timeout_ms);
int set_max_concurrent_tests(int max_tests);
int set_components_to_test(diagnostic_component_t components);
int enable_report_generation(bool enable, const char *directory);

// Получение состояния модуля
const diagnostics_state_t *get_diagnostics_state(void);

// Предустановленные диагностические профили
int run_quick_system_check(void);
int run_comprehensive_system_test(void);
int run_stress_test(int duration_minutes);

// Базовые функции диагностики (без UI)
int run_functionality_tests_core(void);
void perform_diagnostics_core(void);

// Макросы для быстрой диагностики
#define QUICK_DIAG_ALL() run_diagnostics(DIAG_TEST_QUICK, DIAG_COMPONENT_ALL)
#define QUICK_DIAG_CPU() run_diagnostics(DIAG_TEST_QUICK, DIAG_COMPONENT_CPU)
#define QUICK_DIAG_MEMORY() run_diagnostics(DIAG_TEST_QUICK, DIAG_COMPONENT_MEMORY)

// Условная диагностика для релизов
#ifdef ENABLE_DIAGNOSTICS
#define RUN_DIAGNOSTIC(test_type, components) run_diagnostics(test_type, components)
#else
#define RUN_DIAGNOSTIC(test_type, components) 0
#endif

#endif // DIAGNOSTICS_H