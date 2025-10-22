// mac_smc.h - универсальный интерфейс для температур
#ifndef MAC_SMC_H
#define MAC_SMC_H

#include "platform/platform.h"

// Универсальные функции для получения температур
float get_cpu_temperature(void);
float get_fan_speed(void);

// Получение температур ядер CPU
int smc_get_core_temperatures(float *out, int max, int *written);

// Получение температуры памяти
int smc_get_mem_temperature(float *temp);

// Linux альтернативы
#ifndef __APPLE__
#include "platform/linux_hwmon.h"
// Linux использует те же имена функций, но реализация в linux_hwmon.c
#endif

#endif
