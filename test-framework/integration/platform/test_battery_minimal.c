#include <stdio.h>
#include <stdlib.h>
#include "src/battery.h"

int main() {
    printf("Starting battery test...\n");
    
    battery_info_t bat;
    printf("Calling get_battery_info...\n");
    int result = get_battery_info(&bat);
    printf("Battery result: %d\n", result);
    printf("Battery percentage: %.2f\n", bat.percentage);
    printf("Battery charging: %d\n", bat.charging);
    printf("Battery time remaining: %d\n", bat.time_remaining_min);
    
    printf("Battery test completed successfully\n");
    return 0;
}
