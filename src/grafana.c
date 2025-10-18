// grafana.c
#include "grafana.h"
#include <stdio.h>

void grafana_send_metric(const char *metric, float value) {
    // Заглушка — отправка метрики в Grafana
    printf("[GRAFANA] %s = %.2f\n", metric, value);
}
