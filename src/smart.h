#ifndef SMART_H
#define SMART_H

typedef struct {
	int available; // 1 if smartctl found and queried
	int temp_celsius; // -1 if unknown
	char health[32]; // "PASSED"/"FAILED"/"UNKNOWN"
    int power_on_hours; // -1 if unknown
    int reallocated_sectors; // -1 if unknown
    int pending_sectors; // -1 if unknown
    int uncorrectable; // -1 if unknown
} smart_info_t;

// Tries to fetch SMART info using smartctl. device_hint can be NULL to auto-try.
int get_smart_info(const char *device_hint, smart_info_t *out);

#endif






