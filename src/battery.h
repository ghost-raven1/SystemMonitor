#ifndef BATTERY_H
#define BATTERY_H

typedef struct {
	float percentage; // -1 если недоступно
	int charging; // 1/0/-1
	int time_remaining_min; // минуты, -1 если неизвестно
} battery_info_t;

int get_battery_info(battery_info_t *out);
int get_battery_cycle_count(int *cycles);

#endif

