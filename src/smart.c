#include "smart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int try_parse_line(const char *line, smart_info_t *out) {
	if (strstr(line, "Temperature_Celsius")) {
		// ID ATTRIBUTE FLAG VALUE WORST THRESH TYPE UPDATED WHEN_FAILED RAW_VALUE
		const char *p = strrchr(line, ' ');
		if (p) {
			int t = atoi(p);
			if (t > 0 && t < 150) out->temp_celsius = t;
			return 1;
		}
    if (strstr(line, "Power_On_Hours")) {
        const char *p = strrchr(line, ' ');
        if (p) out->power_on_hours = atoi(p);
        return 1;
    }
    if (strstr(line, "Reallocated_Sector_Ct")) {
        const char *p = strrchr(line, ' ');
        if (p) out->reallocated_sectors = atoi(p);
        return 1;
    }
    if (strstr(line, "Current_Pending_Sector")) {
        const char *p = strrchr(line, ' ');
        if (p) out->pending_sectors = atoi(p);
        return 1;
    }
    if (strstr(line, "Offline_Uncorrectable")) {
        const char *p = strrchr(line, ' ');
        if (p) out->uncorrectable = atoi(p);
        return 1;
    }
	}
	if (strstr(line, "SMART overall-health self-assessment test result")) {
		if (strstr(line, ": PASSED")) strncpy(out->health, "PASSED", sizeof(out->health)-1);
		else if (strstr(line, ": FAILED")) strncpy(out->health, "FAILED", sizeof(out->health)-1);
		else strncpy(out->health, "UNKNOWN", sizeof(out->health)-1);
		return 1;
	}
	return 0;
}

int get_smart_info(const char *device_hint, smart_info_t *out) {
	if (!out) return -1;
	out->available = 0;
	out->temp_celsius = -1;
	strncpy(out->health, "UNKNOWN", sizeof(out->health)-1);
    out->power_on_hours = -1;
    out->reallocated_sectors = -1;
    out->pending_sectors = -1;
    out->uncorrectable = -1;

	const char *candidates[] = {
		device_hint ? device_hint : "/dev/disk0",
		"/dev/disk0",
		"/dev/disk1",
		NULL
	};

	// Check smartctl presence
	FILE *chk = popen("which smartctl 2>/dev/null", "r");
	if (!chk) return 0;
	char path[256] = {0};
	if (!fgets(path, sizeof(path), chk)) { pclose(chk); return 0; }
	pclose(chk);
	out->available = 1;

	for (int i = 0; candidates[i]; i++) {
		char cmd[256];
		snprintf(cmd, sizeof(cmd), "smartctl -A %s 2>/dev/null", candidates[i]);
		FILE *fp = popen(cmd, "r");
		if (!fp) continue;
		char line[512];
		while (fgets(line, sizeof(line), fp)) {
			try_parse_line(line, out);
		}
		pclose(fp);
		// Also query health
		snprintf(cmd, sizeof(cmd), "smartctl -H %s 2>/dev/null", candidates[i]);
		fp = popen(cmd, "r");
		if (fp) {
			while (fgets(line, sizeof(line), fp)) {
				try_parse_line(line, out);
			}
			pclose(fp);
		}
		if (out->temp_celsius != -1 || strcmp(out->health, "UNKNOWN") != 0) break;
	}
	return 0;
}





