// notifications.c - Система уведомлений для SysMon
#include "notifications.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "logging.h"

// Структура для хранения настроек уведомлений
typedef struct {
    int cpu_warning_threshold;
    int cpu_critical_threshold;
    int memory_warning_threshold;
    int memory_critical_threshold;
    int temperature_warning_threshold;
    int temperature_critical_threshold;
    char email_recipient[256];
    char slack_webhook[512];
    int notifications_enabled;
    time_t last_notification_time;
} notification_config_t;

static notification_config_t g_config = {
    .cpu_warning_threshold = 75,
    .cpu_critical_threshold = 90,
    .memory_warning_threshold = 85,
    .memory_critical_threshold = 95,
    .temperature_warning_threshold = 70,
    .temperature_critical_threshold = 85,
    .notifications_enabled = 1,
    .last_notification_time = 0
};

void notifications_init(void) {
    // Загружаем настройки уведомлений из переменных окружения
    const char *cpu_warn = getenv("SYSMON_CPU_WARN_THRESHOLD");
    if (cpu_warn) g_config.cpu_warning_threshold = atoi(cpu_warn);

    const char *cpu_crit = getenv("SYSMON_CPU_CRIT_THRESHOLD");
    if (cpu_crit) g_config.cpu_critical_threshold = atoi(cpu_crit);

    const char *mem_warn = getenv("SYSMON_MEM_WARN_THRESHOLD");
    if (mem_warn) g_config.memory_warning_threshold = atoi(mem_warn);

    const char *mem_crit = getenv("SYSMON_MEM_CRIT_THRESHOLD");
    if (mem_crit) g_config.memory_critical_threshold = atoi(mem_crit);

    const char *temp_warn = getenv("SYSMON_TEMP_WARN_THRESHOLD");
    if (temp_warn) g_config.temperature_warning_threshold = atoi(temp_warn);

    const char *temp_crit = getenv("SYSMON_TEMP_CRIT_THRESHOLD");
    if (temp_crit) g_config.temperature_critical_threshold = atoi(temp_crit);

    const char *email = getenv("SYSMON_EMAIL_RECIPIENT");
    if (email) strncpy(g_config.email_recipient, email, sizeof(g_config.email_recipient) - 1);

    const char *slack = getenv("SYSMON_SLACK_WEBHOOK");
    if (slack) strncpy(g_config.slack_webhook, slack, sizeof(g_config.slack_webhook) - 1);

    const char *notif_enabled = getenv("SYSMON_NOTIFICATIONS");
    g_config.notifications_enabled = (notif_enabled && notif_enabled[0] == '1');

    log_info("Notifications system initialized");
}

int send_system_notification(const char *title, const char *message) {
    if (!g_config.notifications_enabled) {
        return 0;
    }

    time_t now = time(NULL);

    // Предотвращаем спам уведомлениями (минимум 5 минут между уведомлениями)
    if (g_config.last_notification_time > 0 &&
        (now - g_config.last_notification_time) < 300) {
        return 0;
    }

    g_config.last_notification_time = now;

    char buf[256];
    snprintf(buf, sizeof(buf), "NOTIFICATION: %s - %s", title, message);
    log_info(buf);

    // Отправляем системное уведомление (macOS)
#ifdef __APPLE__
    char command[1024];
    snprintf(command, sizeof(command),
             "osascript -e 'display notification \"%s\" with title \"SysMon\" subtitle \"%s\"'",
             message, title);

    int result = system(command);
    if (result != 0) {
        log_error("Failed to send system notification");
        return -1;
    }
#endif

    // Отправляем email если настроен получатель
    if (strlen(g_config.email_recipient) > 0) {
        char command[1024];
        snprintf(command, sizeof(command),
                 "echo '%s' | mail -s 'SysMon: %s' %s 2>/dev/null",
                 message, title, g_config.email_recipient);

        int result = system(command);
        if (result != 0) {
            log_error("Failed to send email notification");
        }
    }

    // Отправляем в Slack если настроен webhook
    if (strlen(g_config.slack_webhook) > 0) {
        char command[2048];
        char json_payload[1024];
        snprintf(json_payload, sizeof(json_payload),
                "{\"text\": \"🚨 *SysMon Alert*\\n*Title:* %s\\n*Message:* %s\"}",
                title, message);

        snprintf(command, sizeof(command),
                 "curl -X POST -H 'Content-type: application/json' --data '%s' %s 2>/dev/null",
                 json_payload, g_config.slack_webhook);

        int result = system(command);
        if (result != 0) {
            log_error("Failed to send Slack notification");
        }
    }

    return 0;
}

int check_and_send_alerts(float cpu_usage, float memory_usage, float temperature) {
    if (!g_config.notifications_enabled) {
        return 0;
    }

    int alerts_sent = 0;

    // Проверяем CPU
    if (cpu_usage >= g_config.cpu_critical_threshold) {
        char title[128];
        char message[256];
        snprintf(title, sizeof(title), "Критическая нагрузка CPU");
        snprintf(message, sizeof(message),
                "CPU загружен на %.1f%% (критический порог: %d%%)",
                cpu_usage, g_config.cpu_critical_threshold);

        send_system_notification(title, message);
        alerts_sent++;
    } else if (cpu_usage >= g_config.cpu_warning_threshold) {
        char title[128];
        char message[256];
        snprintf(title, sizeof(title), "Высокая нагрузка CPU");
        snprintf(message, sizeof(message),
                "CPU загружен на %.1f%% (предупреждение: %d%%)",
                cpu_usage, g_config.cpu_warning_threshold);

        send_system_notification(title, message);
        alerts_sent++;
    }

    // Проверяем память
    if (memory_usage >= g_config.memory_critical_threshold) {
        char title[128];
        char message[256];
        snprintf(title, sizeof(title), "Критическое использование памяти");
        snprintf(message, sizeof(message),
                "Память используется на %.1f%% (критический порог: %d%%)",
                memory_usage, g_config.memory_critical_threshold);

        send_system_notification(title, message);
        alerts_sent++;
    } else if (memory_usage >= g_config.memory_warning_threshold) {
        char title[128];
        char message[256];
        snprintf(title, sizeof(title), "Высокое использование памяти");
        snprintf(message, sizeof(message),
                "Память используется на %.1f%% (предупреждение: %d%%)",
                memory_usage, g_config.memory_warning_threshold);

        send_system_notification(title, message);
        alerts_sent++;
    }

    // Проверяем температуру
    if (temperature >= g_config.temperature_critical_threshold) {
        char title[128];
        char message[256];
        snprintf(title, sizeof(title), "Критическая температура");
        snprintf(message, sizeof(message),
                "Температура системы %.1f°C (критический порог: %d°C)",
                temperature, g_config.temperature_critical_threshold);

        send_system_notification(title, message);
        alerts_sent++;
    } else if (temperature >= g_config.temperature_warning_threshold) {
        char title[128];
        char message[256];
        snprintf(title, sizeof(title), "Высокая температура");
        snprintf(message, sizeof(message),
                "Температура системы %.1f°C (предупреждение: %d°C)",
                temperature, g_config.temperature_warning_threshold);

        send_system_notification(title, message);
        alerts_sent++;
    }

    return alerts_sent;
}

void notifications_cleanup(void) {
    log_info("Notifications system cleanup");
}

const notification_config_t *get_notification_config(void) {
    return &g_config;
}
