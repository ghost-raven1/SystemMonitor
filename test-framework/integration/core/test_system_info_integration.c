#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "src/system_info.h"

int main() {
    printf("Testing system_info functions...\n");
    
    // Test CPU usage
    printf("Testing get_cpu_usage()...\n");
    float cpu = get_cpu_usage();
    printf("CPU usage: %.2f%%\n", cpu);
    
    // Test memory usage
    printf("Testing get_memory_usage()...\n");
    float mem = get_memory_usage();
    printf("Memory usage: %.2f%%\n", mem);
    
    // Test uptime
    printf("Testing get_uptime_seconds()...\n");
    long uptime = get_uptime_seconds();
    printf("Uptime: %ld seconds\n", uptime);
    
    printf("System info test completed successfully!\n");
    return 0;
}









