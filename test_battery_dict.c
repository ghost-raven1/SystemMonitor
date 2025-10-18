#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

static int cfnumber_to_int(CFNumberRef num, int *out_val) {
    if (!num || !out_val) return 0;
    if (CFGetTypeID(num) != CFNumberGetTypeID()) return 0;
    CFNumberType t = CFNumberGetType(num);
    switch (t) {
        case kCFNumberIntType:
        case kCFNumberSInt32Type:
        case kCFNumberNSIntegerType:
        case kCFNumberLongType:
        case kCFNumberLongLongType:
            return CFNumberGetValue(num, kCFNumberIntType, out_val);
        default:
            return CFNumberGetValue(num, t, out_val);
    }
}

int main() {
    printf("Testing battery dictionary access...\n");
    
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        printf("Failed to get power info\n");
        return -1;
    }
    
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) {
        printf("Failed to get power sources list\n");
        CFRelease(power_info);
        return -1;
    }
    
    CFIndex count = CFArrayGetCount(list);
    printf("Found %ld power sources\n", count);
    
    for (CFIndex i = 0; i < count; i++) {
        printf("Processing power source %ld...\n", i);
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) continue;
        
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery || CFGetTypeID(battery) != CFDictionaryGetTypeID()) {
            printf("Invalid battery description\n");
            continue;
        }
        
        printf("Testing capacity keys...\n");
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, kIOPSCurrentCapacityKey);
        CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, kIOPSMaxCapacityKey);
        
        printf("Current capacity ref: %p\n", cur);
        printf("Max capacity ref: %p\n", max);
        
        if (cur && max) {
            int cur_i = 0, max_i = 0;
            int cur_result = cfnumber_to_int(cur, &cur_i);
            int max_result = cfnumber_to_int(max, &max_i);
            
            printf("Current: %d (result: %d)\n", cur_i, cur_result);
            printf("Max: %d (result: %d)\n", max_i, max_result);
            
            if (cur_result && max_result && max_i > 0) {
                float percentage = (float)cur_i / (float)max_i * 100.0f;
                printf("Battery percentage: %.1f%%\n", percentage);
            }
        }
        
        printf("Testing charging key...\n");
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, kIOPSIsChargingKey);
        printf("Charging ref: %p\n", charging);
        
        if (charging && CFGetTypeID(charging) == CFBooleanGetTypeID()) {
            int is_charging = CFBooleanGetValue(charging) ? 1 : 0;
            printf("Charging: %d\n", is_charging);
        }
        
        printf("Testing time estimate...\n");
        double est = IOPSGetTimeRemainingEstimate();
        printf("Time estimate: %f\n", est);
        
        break; // Just test the first one
    }
    
    CFRelease(list);
    CFRelease(power_info);
    
    printf("Battery dictionary test completed!\n");
    return 0;
}








