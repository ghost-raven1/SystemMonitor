#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>

int main() {
    printf("Testing battery function step by step...\n");
    
    printf("Step 1: IOPSCopyPowerSourcesInfo...\n");
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        printf("Failed at step 1\n");
        return -1;
    }
    printf("Step 1: Success\n");
    
    printf("Step 2: IOPSCopyPowerSourcesList...\n");
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) {
        printf("Failed at step 2\n");
        CFRelease(power_info);
        return -1;
    }
    printf("Step 2: Success\n");
    
    printf("Step 3: IOPSGetProvidingPowerSourceType...\n");
    CFStringRef ptype = IOPSGetProvidingPowerSourceType(power_info);
    printf("Step 3: Success (ptype: %p)\n", ptype);
    
    if (ptype && CFGetTypeID(ptype) == CFStringGetTypeID()) {
        printf("Step 4: CFStringCompare...\n");
        CFComparisonResult result = CFStringCompare(ptype, CFSTR("Battery Power"), 0);
        printf("Step 4: Success (result: %d)\n", result);
        if (result != kCFCompareEqualTo) {
            printf("Not on battery power, exiting normally\n");
            CFRelease(list);
            CFRelease(power_info);
            return 0;
        }
    }
    
    printf("Step 5: CFArrayGetCount...\n");
    CFIndex count = CFArrayGetCount(list);
    printf("Step 5: Success (count: %ld)\n", count);
    
    printf("Step 6: Loop through power sources...\n");
    for (CFIndex i = 0; i < count; i++) {
        printf("  Processing power source %ld...\n", i);
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) {
            printf("    ps is NULL, continuing...\n");
            continue;
        }
        printf("  About to call IOPSGetPowerSourceDescription...\n");
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery) {
            printf("    battery is NULL, continuing...\n");
            continue;
        }
        printf("    Got battery description\n");
        break; // Just test the first one
    }
    
    printf("Step 6: Success\n");
    
    CFRelease(list);
    CFRelease(power_info);
    
    printf("All steps completed successfully!\n");
    return 0;
}








