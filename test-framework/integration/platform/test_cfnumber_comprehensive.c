#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

int main() {
    printf("Starting comprehensive CFNumber test...\n");
    
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
        
        printf("Accessing dictionary values...\n");
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
        printf("Current capacity: %p\n", cur);
        
        CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));
        printf("Max capacity: %p\n", max);
        
        if (cur && CFGetTypeID(cur) == CFNumberGetTypeID()) {
            printf("Testing current capacity conversion...\n");
            CFNumberType type = CFNumberGetType(cur);
            printf("Current number type: %d\n", type);
            
            int cur_i = 0;
            Boolean success = CFNumberGetValue(cur, kCFNumberIntType, &cur_i);
            printf("Current conversion success: %d, value: %d\n", success, cur_i);
        }
        
        if (max && CFGetTypeID(max) == CFNumberGetTypeID()) {
            printf("Testing max capacity conversion...\n");
            CFNumberType type = CFNumberGetType(max);
            printf("Max number type: %d\n", type);
            
            int max_i = 0;
            Boolean success = CFNumberGetValue(max, kCFNumberIntType, &max_i);
            printf("Max conversion success: %d, value: %d\n", success, max_i);
        }
        
        printf("Power source %ld processed successfully\n", i);
    }
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("Comprehensive CFNumber test completed successfully\n");
    return 0;
}
