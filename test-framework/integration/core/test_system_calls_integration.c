#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <ncurses.h>
#include "src/system_info.h"
#include "src/battery.h"
#include "src/gpu.h"
#include "src/usb.h"

int main() {
    printf("Starting system calls test...\n");
    
    // Test basic system info
    printf("Testing get_cpu_usage...\n");
    float cpu = get_cpu_usage();
    printf("CPU usage: %.2f\n", cpu);
    
    printf("Testing get_memory_usage...\n");
    float mem = get_memory_usage();
    printf("Memory usage: %.2f\n", mem);
    
    printf("Testing get_uptime_seconds...\n");
    long uptime = get_uptime_seconds();
    printf("Uptime: %ld\n", uptime);
    
    printf("Testing get_cpu_frequencies...\n");
    long long fcur = -1, fmax = -1;
    get_cpu_frequencies(&fcur, &fmax);
    printf("CPU frequencies: %lld %lld\n", fcur, fmax);
    
    // Test battery (disabled)
    printf("Testing battery (disabled)...\n");
    battery_info_t bat;
    int result = get_battery_info(&bat);
    printf("Battery result: %d\n", result);
    
    // Test GPU (disabled)
    printf("Testing GPU (disabled)...\n");
    gpu_info_t gpu;
    result = get_gpu_info(&gpu);
    printf("GPU result: %d\n", result);
    
    // Test USB (disabled)
    printf("Testing USB (disabled)...\n");
    usb_device_t devices[10];
    int count = 0;
    result = list_usb_devices(devices, 10, &count);
    printf("USB result: %d, count: %d\n", result, count);
    
    printf("All tests completed successfully\n");
    return 0;
}
