// prometheus.c
#include "prometheus.h"
#include <stdio.h>

void prometheus_export_metric(const char *name, float value) {
    printf("[PROMETHEUS] %s = %.2f\n", name, value);
}
