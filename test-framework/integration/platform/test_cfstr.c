#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

int main() {
    printf("Starting CFSTR test...\n");
    
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
        
        printf("Testing CFSTR macro...\n");
        CFStringRef charging_key = CFSTR(kIOPSIsChargingKey);
        printf("Charging key: %p\n", charging_key);
        
        printf("Getting charging status with CFSTR...\n");
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, charging_key);
        printf("Charging: %p\n", charging);
        
        if (charging && CFGetTypeID(charging) == CFBooleanGetTypeID()) {
            printf("About to call CFBooleanGetValue...\n");
            Boolean is_charging = CFBooleanGetValue(charging);
            printf("CFBooleanGetValue succeeded, value: %d\n", is_charging);
        }
        
        printf("Power source %ld processed successfully\n", i);
    }
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("CFSTR test completed successfully\n");
    return 0;
}
