#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPSKeys.h>

int main() {
    printf("Testing IOPS key constants...\n");
    
    printf("kIOPSCurrentCapacityKey: %p\n", kIOPSCurrentCapacityKey);
    printf("kIOPSMaxCapacityKey: %p\n", kIOPSMaxCapacityKey);
    printf("kIOPSIsChargingKey: %p\n", kIOPSIsChargingKey);
    
    // Try to get the string values
    if (kIOPSCurrentCapacityKey && CFGetTypeID(kIOPSCurrentCapacityKey) == CFStringGetTypeID()) {
        char str[256];
        if (CFStringGetCString(kIOPSCurrentCapacityKey, str, sizeof(str), kCFStringEncodingUTF8)) {
            printf("kIOPSCurrentCapacityKey value: %s\n", str);
        }
    }
    
    if (kIOPSMaxCapacityKey && CFGetTypeID(kIOPSMaxCapacityKey) == CFStringGetTypeID()) {
        char str[256];
        if (CFStringGetCString(kIOPSMaxCapacityKey, str, sizeof(str), kCFStringEncodingUTF8)) {
            printf("kIOPSMaxCapacityKey value: %s\n", str);
        }
    }
    
    if (kIOPSIsChargingKey && CFGetTypeID(kIOPSIsChargingKey) == CFStringGetTypeID()) {
        char str[256];
        if (CFStringGetCString(kIOPSIsChargingKey, str, sizeof(str), kCFStringEncodingUTF8)) {
            printf("kIOPSIsChargingKey value: %s\n", str);
        }
    }
    
    printf("Constants test completed!\n");
    return 0;
}









