// prometheus.h
#ifndef PROMETHEUS_H
#define PROMETHEUS_H

void prometheus_export_metric(const char *name, float value);
const char *prometheus_get_metrics(void);
int prometheus_start_server(int port);

#endif
