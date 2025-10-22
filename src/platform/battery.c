#include "platform/battery.h"
#include "platform/platform.h"
#include "utils/logging.h"
#include "utils/error_handler.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/IOKitLib.h>
#else
// Linux версия будет подключена через условную компиляцию
#endif

#ifdef __APPLE__
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

    // Инициализируем значения по умолчанию
    out->percentage = -1.0f;
    out->charging = -1;
    out->time_remaining_min = -1;

    // Quick exit in safe mode
    const char *safe = getenv("SYSMON_SAFE");
    if (safe && safe[0]=='1') return 0;

    // Respect explicit disable
    const char *no_bat = getenv("SYSMON_NO_BAT");
    if (no_bat && no_bat[0]=='1') return 0;

    // Безопасная работа с CoreFoundation
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        LOG_ERROR(ERR_BATTERY_INIT, "Failed to get power sources info", "IOPSCopyPowerSourcesInfo");
        return -1;
    }

    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) {
        CFRelease(power_info);
        LOG_ERROR(ERR_BATTERY_READ, "Failed to get power sources list", "IOPSCopyPowerSourcesList");
        return -1;
    }

    // Безопасная проверка типа источника питания
    CFStringRef ptype = IOPSGetProvidingPowerSourceType(power_info);
    if (ptype && CFGetTypeID(ptype) == CFStringGetTypeID()) {
        if (CFStringCompare(ptype, CFSTR("Battery Power"), 0) != kCFCompareEqualTo) {
            CFRelease(list);
            CFRelease(power_info);
            return 0; // Не на батарее
        }
    }

    CFIndex count = CFArrayGetCount(list);
    if (count <= 0) {
        CFRelease(list);
        CFRelease(power_info);
        return -1;
    }

    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        if (!ps) continue;

        CFDictionaryRef battery = IOPSGetPowerSourceDescription(power_info, ps);
        if (!battery || CFGetTypeID(battery) != CFDictionaryGetTypeID()) continue;

        // Проверяем что это действительно батарея
        CFTypeRef type = CFDictionaryGetValue(battery, CFSTR(kIOPSTypeKey));
        if (!type || CFGetTypeID(type) != CFStringGetTypeID()) continue;

        CFStringRef battery_type = (CFStringRef)type;
        if (CFStringCompare(battery_type, CFSTR("InternalBattery"), 0) != kCFCompareEqualTo &&
           CFStringCompare(battery_type, CFSTR("UPSBattery"), 0) != kCFCompareEqualTo) {
            continue; // Не батарея
        }

        // Получаем текущую емкость
        CFNumberRef cur = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
        if (cur && CFGetTypeID(cur) == CFNumberGetTypeID()) {
            int cur_i = 0;
            if (cfnumber_to_int(cur, &cur_i) && cur_i >= 0) {
                // Получаем максимальную емкость
                CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));
                if (max && CFGetTypeID(max) == CFNumberGetTypeID()) {
                    int max_i = 0;
                    if (cfnumber_to_int(max, &max_i) && max_i > 0) {
                        out->percentage = (float)cur_i / (float)max_i * 100.0f;
                    }
                }
            }
        }

        // Получаем статус зарядки
        CFBooleanRef charging = (CFBooleanRef)CFDictionaryGetValue(battery, CFSTR(kIOPSIsChargingKey));
        if (charging && CFGetTypeID(charging) == CFBooleanGetTypeID()) {
            out->charging = CFBooleanGetValue(charging) ? 1 : 0;
        }

        // Получаем время до разряда (безопасно)
        CFNumberRef tte = (CFNumberRef)CFDictionaryGetValue(battery, CFSTR(kIOPSTimeToEmptyKey));
        if (tte && CFGetTypeID(tte) == CFNumberGetTypeID()) {
            int minutes = 0;
            if (cfnumber_to_int(tte, &minutes) && minutes > 0) {
                out->time_remaining_min = minutes;
            }
        }

        break; // Используем первый подходящий источник
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

#endif // __APPLE__
