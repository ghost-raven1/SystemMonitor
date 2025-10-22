#include "platform/gpu.h"
#include "platform/platform.h"
#include "utils/logging.h"

#ifdef __APPLE__
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#else
#include <stdio.h>
#include <string.h>
#endif

int get_gpu_info(gpu_info_t *out) {
	if (!out) return -1;

	// Инициализируем значения по умолчанию
	out->usage = -1.0f;
	out->temperature = -1.0f;
	out->memory_used = 0;
	out->memory_total = 0;

	// Проверяем отключение модуля
	const char *no_gpu = getenv("SYSMON_NO_GPU");
	const char *safe = getenv("SYSMON_SAFE");
	if ((no_gpu && no_gpu[0] == '1') || (safe && safe[0] == '1')) {
		return 0; // Модуль отключен
	}

#ifdef __APPLE__
	// macOS реализация через IOKit
	io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOAccelerator"));
	if (service) {
		CFMutableDictionaryRef props = NULL;
		if (IORegistryEntryCreateCFProperties(service, &props, kCFAllocatorDefault, 0) == KERN_SUCCESS && props) {
			// Попытка получить статистику производительности
			CFTypeRef perf = CFDictionaryGetValue(props, CFSTR("PerformanceStatistics"));
			if (perf && CFGetTypeID(perf) == CFDictionaryGetTypeID()) {
				CFNumberRef util = CFDictionaryGetValue((CFDictionaryRef)perf, CFSTR("Device Utilization %"));
				if (util && CFGetTypeID(util) == CFNumberGetTypeID()) {
					float usage = -1.0f;
					if (CFNumberGetValue(util, kCFNumberFloatType, &usage)) {
						out->usage = usage;
					}
				}

				// Попытка получить температуру GPU
				CFNumberRef temp = CFDictionaryGetValue((CFDictionaryRef)perf, CFSTR("Temperature"));
				if (temp && CFGetTypeID(temp) == CFNumberGetTypeID()) {
					int temp_celsius = -1;
					if (CFNumberGetValue(temp, kCFNumberIntType, &temp_celsius)) {
						out->temperature = (float)temp_celsius;
					}
				}
			}

			// Попытка получить информацию о памяти
			CFTypeRef mem_info = CFDictionaryGetValue(props, CFSTR("VRAM"));
			if (mem_info && CFGetTypeID(mem_info) == CFDictionaryGetTypeID()) {
				CFNumberRef mem_total = CFDictionaryGetValue((CFDictionaryRef)mem_info, CFSTR("total"));
				if (mem_total && CFGetTypeID(mem_total) == CFNumberGetTypeID()) {
					long long mem_total_bytes = 0;
					if (CFNumberGetValue(mem_total, kCFNumberLongLongType, &mem_total_bytes)) {
						out->memory_total = (int)(mem_total_bytes / (1024 * 1024)); // В MB
					}
				}

				CFNumberRef mem_used = CFDictionaryGetValue((CFDictionaryRef)mem_info, CFSTR("used"));
				if (mem_used && CFGetTypeID(mem_used) == CFNumberGetTypeID()) {
					long long mem_used_bytes = 0;
					if (CFNumberGetValue(mem_used, kCFNumberLongLongType, &mem_used_bytes)) {
						out->memory_used = (int)(mem_used_bytes / (1024 * 1024)); // В MB
					}
				}
			}

			CFRelease(props);
		}
		IOObjectRelease(service);
	}

	// Fallback: попытка получить информацию через системный профайлер
	if (out->usage < 0) {
		// Попытка использовать iStats CLI утилиту если доступна
		FILE *fp = popen("which istats 2>/dev/null", "r");
		if (fp) {
			int c = fgetc(fp);
			if (c != EOF) {
				// iStats доступен, пробуем получить GPU usage
				FILE *gpu_fp = popen("istats gpu --value-only 2>/dev/null", "r");
				if (gpu_fp) {
					float gpu_usage = -1.0f;
					if (fscanf(gpu_fp, "%f", &gpu_usage) == 1 && gpu_usage >= 0) {
						out->usage = gpu_usage;
					}
					pclose(gpu_fp);
				}
			}
			pclose(fp);
		}
	}

#else
	// Linux реализация
	// Попытка получить информацию через /sys/class/drm
	FILE *fp = fopen("/sys/class/drm/card0/device/gpu_busy_percent", "r");
	if (fp) {
		if (fscanf(fp, "%f", &out->usage) == 1) {
			// Успешно прочитали usage
		}
		fclose(fp);
	}

	// Попытка получить температуру GPU
	fp = fopen("/sys/class/drm/card0/device/hwmon/hwmon*/temp1_input", "r");
	if (fp) {
		int temp_millidegrees = 0;
		if (fscanf(fp, "%d", &temp_millidegrees) == 1) {
			out->temperature = (float)temp_millidegrees / 1000.0f;
		}
		fclose(fp);
	}

	// Попытка получить информацию о памяти через nvidia-ml или fglrx
	fp = popen("nvidia-smi --query-gpu=memory.used,memory.total --format=csv,noheader,nounits 2>/dev/null | head -n1", "r");
	if (fp) {
		int mem_used_mb, mem_total_mb;
		if (fscanf(fp, "%d, %d", &mem_used_mb, &mem_total_mb) == 2) {
			out->memory_used = mem_used_mb;
			out->memory_total = mem_total_mb;
		}
		pclose(fp);
	}
#endif

	return 0;
}


