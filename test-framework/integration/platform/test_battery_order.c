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
    printf("Starting battery order test...\n");
    
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        printf("IOPSCopyPowerSourcesInfo returned NULL\n");
        return 1;
    }
    
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) { CFRelease(power_info); return -1; }

    CFIndex count = CFArrayGetCount(list);
    printf("Array count: %ld\n", count);
    
    for (CFIndex i = 0; i < count; i++) {
        printf("Processing power source %ld...\n", i);
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) continue;
        
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery || CFGetTypeID(battery) != CFDictionaryGetTypeID()) continue;

        // Proceed if it looks like a battery (has capacity keys)

        printf("Step 1: Getting capacity values...\n");
        int cur_i = 0, max_i = 0;
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
        CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));
        printf("Capacity values obtained\n");
        
        printf("Step 2: Converting capacity values...\n");
        if (cfnumber_to_int(cur, &cur_i) && cfnumber_to_int(max, &max_i) && max_i > 0) {
            float percentage = (float)cur_i / (float)max_i * 100.0f;
            printf("Percentage calculated: %.2f\n", percentage);
        }

        printf("Step 3: Getting charging status...\n");
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        printf("Charging obtained: %p\n", charging);
        
        printf("Step 4: Checking charging type...\n");
        if (charging && CFGetTypeID(charging) == CFBooleanGetTypeID()) {
            printf("Charging is valid CFBoolean\n");
            printf("Step 5: Getting charging value...\n");
            Boolean is_charging = CFBooleanGetValue(charging);
            printf("Charging value: %d\n", is_charging);
        }

        printf("Step 6: Getting time to empty...\n");
        CFNumberRef tte = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSTimeToEmptyKey));
        printf("Time to empty obtained: %p\n", tte);
        
        printf("Power source %ld processed successfully\n", i);
        break; // Use the first suitable power source
    }

    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("Battery order test completed successfully\n");
    return 0;
}
