#ifndef ANOMALIES_H
#define ANOMALIES_H

#define ANOMALY_WINDOW 10

typedef struct {
	float cpu[ANOMALY_WINDOW];
	float mem[ANOMALY_WINDOW];
	int index;
} anomaly_history_t;

void anomalies_init(anomaly_history_t *h);
int anomalies_push(anomaly_history_t *h, float cpu_usage, float mem_percent, float threshold_sigma);

#endif

