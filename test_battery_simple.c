#include <stdio.h>
#include <stdlib.h>
#include "src/battery.h"

int main() {
    printf("Testing basic battery function...\n");
    
    battery_info_t bat;
    printf("About to call get_battery_info...\n");
    int result = get_battery_info(&bat);
    printf("get_battery_info returned: %d\n", result);
    if (result == 0) {
        printf("Battery: %.1f%%, charging: %d, time: %d min\n", 
               bat.percentage, bat.charging, bat.time_remaining_min);
    }
    
    printf("Basic battery test completed!\n");
    return 0;
}








