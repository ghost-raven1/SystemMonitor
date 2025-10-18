// mac_smc.h
#ifndef MAC_SMC_H
#define MAC_SMC_H

float get_cpu_temperature();
float get_fan_speed();
// Optional: per-core and RAM temps; return -1.0f if unavailable
int smc_get_core_temperatures(float *out, int max, int *written);
int smc_get_mem_temperature(float *temp);

#endif
