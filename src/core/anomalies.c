#include "core/anomalies.h"
#include <math.h>

void anomalies_init(anomaly_history_t *h) {
	if (!h) return;
	for (int i = 0; i < ANOMALY_WINDOW; i++) { h->cpu[i] = 0.0f; h->mem[i] = 0.0f; }
	h->index = 0;
}

static float mean(const float *a) {
	float s = 0.0f; for (int i = 0; i < ANOMALY_WINDOW; i++) s += a[i]; return s / ANOMALY_WINDOW;
}
static float stddev(const float *a, float m) {
	float v = 0.0f; for (int i = 0; i < ANOMALY_WINDOW; i++) { float d = a[i]-m; v += d*d; } return sqrtf(v / ANOMALY_WINDOW);
}

int anomalies_push(anomaly_history_t *h, float cpu_usage, float mem_percent, float threshold_sigma) {
	if (!h) return 0;
	h->cpu[h->index] = cpu_usage;
	h->mem[h->index] = mem_percent;
	h->index = (h->index + 1) % ANOMALY_WINDOW;
	if (h->index != 0) return 0; // ждём заполнения окна
	float mc = mean(h->cpu), mm = mean(h->mem);
	float sc = stddev(h->cpu, mc), sm = stddev(h->mem, mm);
	int flags = 0;
	if (sc > 0.0f && cpu_usage > mc + threshold_sigma * sc) flags |= 1;
	if (sm > 0.0f && mem_percent > mm + threshold_sigma * sm) flags |= 2;
	return flags;
}


