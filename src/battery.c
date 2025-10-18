#include "battery.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/IOKitLib.h>

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

    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) return -1;
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) { CFRelease(power_info); return -1; }

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

    CFIndex count = CFArrayGetCount(list);
    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) continue;
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery || CFGetTypeID(battery) != CFDictionaryGetTypeID()) continue;

        // Proceed if it looks like a battery (has capacity keys)

        int cur_i = 0, max_i = 0;
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
        CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));
        if (cfnumber_to_int(cur, &cur_i) && cfnumber_to_int(max, &max_i) && max_i > 0) {
            out->percentage = (float)cur_i / (float)max_i * 100.0f;
        }

        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        if (charging && CFGetTypeID(charging) == CFBooleanGetTypeID()) {
            out->charging = CFBooleanGetValue(charging) ? 1 : 0;
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
            CFNumberRef tte = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSTimeToEmptyKey));
            (void)cfnumber_to_int(tte, &out->time_remaining_min);
        //}
        break; // Use the first suitable power source
    }

    CFRelease(list);
    CFRelease(power_info);
    return 0;
}

int get_battery_cycle_count(int *cycles) {
    if (!cycles) return -1;
    *cycles = -1;
    io_registry_entry_t entry = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceNameMatching("AppleSmartBattery"));
    if (!entry) return -1;
    CFNumberRef num = IORegistryEntryCreateCFProperty(entry, CFSTR("CycleCount"), kCFAllocatorDefault, 0);
    if (num) {
        int c = -1;
        (void)cfnumber_to_int(num, &c);
        if (c >= 0) *cycles = c;
        CFRelease(num);
    }
    IOObjectRelease(entry);
    return 0;
}


