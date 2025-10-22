#ifndef LINUX_HWMON_H
#define LINUX_HWMON_H

// Linux hwmon interface для получения информации о железе

// Получение температуры CPU
float get_cpu_temperature(void);

// Получение температур ядер CPU
int linux_get_core_temperatures(float *out, int max, int *written);

// Получение температуры памяти
int linux_get_mem_temperature(float *temp);

// Получение скорости вентиляторов
float get_fan_speed(void);

#endif // LINUX_HWMON_H
