#include "gpu.h"
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

int get_gpu_info(gpu_info_t *out) {
	if (!out) return -1;
	out->usage = -1.0f;
	out->temperature = -1.0f;
	out->memory_used = 0;
	out->memory_total = 0;

    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOAccelerator"));
    if (service) {
        CFMutableDictionaryRef props = NULL;
        if (IORegistryEntryCreateCFProperties(service, &props, kCFAllocatorDefault, 0) == KERN_SUCCESS && props) {
            CFTypeRef perf = CFDictionaryGetValue(props, CFSTR("PerformanceStatistics"));
            if (perf && CFGetTypeID(perf) == CFDictionaryGetTypeID()) {
                CFNumberRef util = CFDictionaryGetValue((CFDictionaryRef)perf, CFSTR("Device Utilization %"));
                if (util) CFNumberGetValue(util, kCFNumberFloatType, &out->usage);
            }
            CFRelease(props);
        }
        IOObjectRelease(service);
    }
	return 0;
}


