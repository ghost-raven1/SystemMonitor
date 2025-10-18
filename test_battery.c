#include <stdio.h>
#include <stdlib.h>
#include "src/battery.h"

int main() {
    printf("Testing battery module...\n");
    
    battery_info_t bat;
    int result = get_battery_info(&bat);
    printf("get_battery_info returned: %d\n", result);
    if (result == 0) {
        printf("Battery: %.1f%%, charging: %d, time: %d min\n", 
               bat.percentage, bat.charging, bat.time_remaining_min);
    }
    
    int cycles = -1;
    result = get_battery_cycle_count(&cycles);
    printf("get_battery_cycle_count returned: %d, cycles: %d\n", result, cycles);
    
    printf("Battery test completed!\n");
    return 0;
}








