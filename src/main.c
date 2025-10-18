// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include "ui.h"
#include "config.h"

int main() {
    const char *cfg = getenv("SYSMON_CONFIG");
    if (cfg) {
        load_config(cfg);
    } else {
        if (load_config("src/config.ini") != 0) {
            load_config("config.ini");
        }
    }
    // No forced defaults: respect user's environment entirely
    run_ui();
    return 0;
}
