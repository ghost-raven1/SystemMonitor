#ifndef GPU_H
#define GPU_H

typedef struct {
	float usage; // -1 если недоступно
	float temperature; // -1 если недоступно
	unsigned long long memory_used;
	unsigned long long memory_total;
} gpu_info_t;

int get_gpu_info(gpu_info_t *out);

#endif

