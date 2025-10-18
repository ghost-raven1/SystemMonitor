#include "usb.h"
#include "platform.h"

#ifdef __APPLE__
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/usb/USB.h>
#include <string.h>
#endif

#ifdef __APPLE__
int list_usb_devices(usb_device_t *out, int max, int *count) {
	if (!out || max <= 0 || !count) return -1;
	*count = 0;
	io_iterator_t iter = IO_OBJECT_NULL;
	kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOUSBDevice"), &iter);
	if (kr != KERN_SUCCESS) return -1;
	io_service_t device;
    while ((device = IOIteratorNext(iter)) && *count < max) {
		usb_device_t *d = &out[*count];
		d->name[0] = '\0';
        CFStringRef key = CFStringCreateWithCString(kCFAllocatorDefault, kUSBProductString, kCFStringEncodingUTF8);
        CFStringRef name = NULL;
        if (key) {
            CFTypeRef v = IORegistryEntryCreateCFProperty(device, key, kCFAllocatorDefault, 0);
            if (v && CFGetTypeID(v) == CFStringGetTypeID()) {
                name = (CFStringRef)v; // take ownership
            } else if (v) {
                CFRelease(v);
            }
            CFRelease(key);
        }
		if (name) {
			CFStringGetCString(name, d->name, sizeof(d->name), kCFStringEncodingUTF8);
			CFRelease(name);
		} else {
			strncpy(d->name, "USB Device", sizeof(d->name) - 1);
		}
		strncpy(d->type, "USB", sizeof(d->type) - 1);
		d->speed_mbps = 0.0f;
		(*count)++;
		IOObjectRelease(device);
	}
	IOObjectRelease(iter);
	return 0;
}
#endif // __APPLE__
