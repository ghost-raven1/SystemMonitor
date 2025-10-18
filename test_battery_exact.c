#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

typedef struct {
    float percentage;
    int charging;
    int time_remaining_min;
} battery_info_t;

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

int get_battery_info(battery_info_t *out) {
    if (!out) return -1;
    out->percentage = -1.0f;
    out->charging = -1;
    out->time_remaining_min = -1;
    
    // Quick exit in safe mode
    const char *safe = getenv("SYSMON_SAFE");
    if (safe && safe[0]=='1') return 0;
    // Respect explicit disable
    const char *no_bat = getenv("SYSMON_NO_BAT");
    if (no_bat && no_bat[0]=='1') return 0;

    printf("Calling IOPSCopyPowerSourcesInfo...\n");
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) return -1;
    printf("IOPSCopyPowerSourcesInfo succeeded\n");
    
    printf("Calling IOPSCopyPowerSourcesList...\n");
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) { CFRelease(power_info); return -1; }
    printf("IOPSCopyPowerSourcesList succeeded\n");

    // Skip if not running on battery power and no need to query details
    // Note: IOPSGetProvidingPowerSourceType might not be available on all systems
    // Commenting out this check to avoid potential segfaults
    /*
    CFStringRef ptype = IOPSGetProvidingPowerSourceType(power_info);
    if (ptype && CFGetTypeID(ptype) == CFStringGetTypeID()) {
        if (CFStringCompare(ptype, CFSTR("Battery Power"), 0) != kCFCompareEqualTo) {
            CFRelease(list);
            CFRelease(power_info);
            return 0;
        }
    }
    */

    printf("Getting array count...\n");
    CFIndex count = CFArrayGetCount(list);
    printf("Array count: %ld\n", count);
    
    for (CFIndex i = 0; i < count; i++) {
        printf("Processing power source %ld...\n", i);
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) continue;
        
        printf("Getting power source description...\n");
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery || CFGetTypeID(battery) != CFDictionaryGetTypeID()) continue;
        printf("Power source description obtained\n");

        // Proceed if it looks like a battery (has capacity keys)

        printf("Getting capacity values...\n");
        int cur_i = 0, max_i = 0;
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
        CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));
        printf("Capacity values obtained\n");
        
        if (cfnumber_to_int(cur, &cur_i) && cfnumber_to_int(max, &max_i) && max_i > 0) {
            out->percentage = (float)cur_i / (float)max_i * 100.0f;
            printf("Percentage calculated: %.2f\n", out->percentage);
        }

        printf("Getting charging status...\n");
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        if (charging && CFGetTypeID(charging) == CFBooleanGetTypeID()) {
            out->charging = CFBooleanGetValue(charging) ? 1 : 0;
            printf("Charging status: %d\n", out->charging);
        }

        // Prefer API estimate when available
        // Note: IOPSGetTimeRemainingEstimate might be causing segfaults
        // Commenting out this call to avoid potential issues
        /*
        double est = IOPSGetTimeRemainingEstimate();
        if (est != kIOPSTimeRemainingUnknown && est != kIOPSTimeRemainingUnlimited) {
            out->time_remaining_min = (int)(est / 60.0);
        } else {
        */
            printf("Getting time to empty...\n");
            CFNumberRef tte = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSTimeToEmptyKey));
            (void)cfnumber_to_int(tte, &out->time_remaining_min);
            printf("Time to empty: %d\n", out->time_remaining_min);
        //}
        break; // Use the first suitable power source
    }

    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    printf("Resources released\n");
    return 0;
}

int main() {
    printf("Starting exact battery test...\n");
    
    battery_info_t bat;
    int result = get_battery_info(&bat);
    printf("Battery result: %d\n", result);
    printf("Battery percentage: %.2f\n", bat.percentage);
    printf("Battery charging: %d\n", bat.charging);
    printf("Battery time remaining: %d\n", bat.time_remaining_min);
    
    printf("Exact battery test completed successfully\n");
    return 0;
}
