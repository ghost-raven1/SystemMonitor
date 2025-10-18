#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

int main() {
    printf("Starting charging debug test...\n");
    
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
        
        printf("Getting charging status...\n");
        CFTypeRef charging = CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        printf("Charging: %p\n", charging);
        
        if (charging) {
            printf("Charging type ID: %lu\n", CFGetTypeID(charging));
            printf("Charging is CFBoolean: %d\n", CFGetTypeID(charging) == CFBooleanGetTypeID());
            printf("Charging is CFNumber: %d\n", CFGetTypeID(charging) == CFNumberGetTypeID());
            printf("Charging is CFString: %d\n", CFGetTypeID(charging) == CFStringGetTypeID());
            
            if (CFGetTypeID(charging) == CFNumberGetTypeID()) {
                printf("Charging is a CFNumber, trying to get value...\n");
                int charging_int = 0;
                Boolean success = CFNumberGetValue((CFNumberRef)charging, kCFNumberIntType, &charging_int);
                printf("Charging number conversion success: %d, value: %d\n", success, charging_int);
            }
        }
        
        printf("Power source %ld processed successfully\n", i);
    }
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("Charging debug test completed successfully\n");
    return 0;
}
