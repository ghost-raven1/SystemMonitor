#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

int main() {
    printf("Starting CFBoolean test...\n");
    
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        printf("IOPSCopyPowerSourcesInfo returned NULL\n");
        return 1;
    }
    
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) {
        printf("IOPSCopyPowerSourcesList returned NULL\n");
        CFRelease(power_info);
        return 1;
    }
    
    CFIndex count = CFArrayGetCount(list);
    printf("Array count: %ld\n", count);
    
    for (CFIndex i = 0; i < count; i++) {
        printf("Processing power source %ld...\n", i);
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) continue;
        
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery || CFGetTypeID(battery) != CFDictionaryGetTypeID()) continue;
        
        printf("Accessing charging status...\n");
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        printf("Charging: %p\n", charging);
        
        if (charging) {
            printf("Checking charging type...\n");
            printf("Charging type ID: %lu\n", CFGetTypeID(charging));
            printf("Charging is CFBoolean: %d\n", CFGetTypeID(charging) == CFBooleanGetTypeID());
            
            if (CFGetTypeID(charging) == CFBooleanGetTypeID()) {
                printf("Getting charging value...\n");
                Boolean is_charging = CFBooleanGetValue(charging);
                printf("Charging value: %d\n", is_charging);
            }
        }
        
        printf("Power source %ld processed successfully\n", i);
    }
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("CFBoolean test completed successfully\n");
    return 0;
}
