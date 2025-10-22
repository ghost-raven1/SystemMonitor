#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

int main() {
    printf("Starting dictionary access test...\n");
    
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
        if (!ps) {
            printf("Power source %ld is NULL\n", i);
            continue;
        }
        
        printf("Getting power source description...\n");
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery) {
            printf("Power source description is NULL\n");
            continue;
        }
        
        printf("Checking dictionary type...\n");
        if (CFGetTypeID(battery) != CFDictionaryGetTypeID()) {
            printf("Power source description is not a dictionary\n");
            continue;
        }
        
        printf("Accessing dictionary values...\n");
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
        printf("Current capacity: %p\n", cur);
        
        CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));
        printf("Max capacity: %p\n", max);
        
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        printf("Charging: %p\n", charging);
        
        printf("Power source %ld processed successfully\n", i);
    }
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("Dictionary access test completed successfully\n");
    return 0;
}
