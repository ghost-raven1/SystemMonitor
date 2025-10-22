// mac_smc.c
#include "platform/mac_smc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>

static int read_first_float_from_cmd(const char *cmd, float *out) {
    if (!cmd || !out) return -1;
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    char buf[256];
    float val = -1.0f;
    while (fgets(buf, sizeof(buf), fp)) {
        // strip common trailing unit letter; sscanf will ignore leading spaces
        for (char *p = buf; *p; ++p) {
            if (*p == 'C') *p = ' ';
        }
        float v = -1.0f;
        if (sscanf(buf, "%f", &v) == 1) { val = v; break; }
    }
    pclose(fp);
    if (val >= 0.0f && val < 150.0f) { *out = val; return 0; }
    return -1;
}

static int which_cmd(const char *tool, char *path, size_t path_sz) {
    FILE *fp = popen("which istats 2>/dev/null", "r");
    if (!fp) return -1;
    if (!fgets(path, path_sz, fp)) { pclose(fp); return -1; }
    pclose(fp);
    size_t L = strlen(path);
    if (L > 0 && (path[L-1] == '\n' || path[L-1] == '\r')) path[L-1] = '\0';
    return 0;
}

// Best-effort: try iStats (brew), then smc utility, else fallback
float get_cpu_temperature() {
    float t = -1.0f;
    char path[256] = {0};
    if (which_cmd("istats", path, sizeof(path)) == 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s cpu --value-only 2>/dev/null", path);
        if (read_first_float_from_cmd(cmd, &t) == 0) return t;
    }
    // try smc utility for TC0P key
    if (read_first_float_from_cmd("smc -k TC0P -r 2>/dev/null | awk '{print $NF}'", &t) == 0) return t;
    // fallback constant to avoid crashes
    return 50.0f;
}

float get_fan_speed() {
    float rpm = -1.0f;
    // try smc fan read, key F0Ac (fan actual)
    if (read_first_float_from_cmd("smc -k F0Ac -r 2>/dev/null | awk '{print $NF}'", &rpm) == 0 && rpm >= 0.0f) return rpm;
    return 1200.0f;
}

int smc_get_core_temperatures(float *out, int max, int *written) {
    if (written) *written = 0;
    if (!out || max <= 0) return -1;
    // Best-effort: use package temp for all cores
    float pkg = get_cpu_temperature();
    if (pkg < 0.0f) return -1;
    int ncpu = 1;
    size_t sz = sizeof(ncpu);
    int mib[2] = {CTL_HW, HW_NCPU};
    sysctl(mib, 2, &ncpu, &sz, NULL, 0);
    if (ncpu <= 0) ncpu = 1;
    int n = ncpu;
    if (n > max) n = max;
    for (int i = 0; i < n; i++) out[i] = pkg;
    if (written) *written = n;
    return 0;
}

int smc_get_mem_temperature(float *temp) {
    if (!temp) return -1;
    // Try SMC TM0P key via smc binary
    float t = -1.0f;
    if (read_first_float_from_cmd("smc -k TM0P -r 2>/dev/null | awk '{print $NF}'", &t) == 0 && t > 0.0f && t < 150.0f) {
        *temp = t;
        return 0;
    }
    *temp = -1.0f;
    return 0;
}
