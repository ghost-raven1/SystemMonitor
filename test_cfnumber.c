#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

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
    printf("Starting CFNumber test...\n");
    
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
        
        printf("Testing cfnumber_to_int...\n");
        int cur_i = 0, max_i = 0;
        int cur_result = cfnumber_to_int(cur, &cur_i);
        int max_result = cfnumber_to_int(max, &max_i);
        
        printf("Current: result=%d, value=%d\n", cur_result, cur_i);
        printf("Max: result=%d, value=%d\n", max_result, max_i);
        
        if (cur_result && max_result && max_i > 0) {
            float percentage = (float)cur_i / (float)max_i * 100.0f;
            printf("Battery percentage: %.2f%%\n", percentage);
        }
        
        printf("Power source %ld processed successfully\n", i);
    }
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("CFNumber test completed successfully\n");
    return 0;
}
