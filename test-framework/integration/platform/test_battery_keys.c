#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>

int main() {
    printf("Testing available battery keys...\n");
    
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
        
        printf("Listing all keys in battery dictionary...\n");
        
        // Get all keys from the dictionary
        CFIndex key_count = CFDictionaryGetCount(battery);
        printf("Found %ld keys:\n", key_count);
        
        // Get all keys
        CFStringRef *keys = malloc(key_count * sizeof(CFStringRef));
        if (keys) {
            CFDictionaryGetKeysAndValues(battery, (const void **)keys, NULL);
            
            for (CFIndex j = 0; j < key_count; j++) {
                CFStringRef key = keys[j];
                if (key && CFGetTypeID(key) == CFStringGetTypeID()) {
                    char key_str[256];
                    if (CFStringGetCString(key, key_str, sizeof(key_str), kCFStringEncodingUTF8)) {
                        printf("  Key %ld: %s\n", j, key_str);
                    }
                }
            }
            free(keys);
        }
        
        break; // Just test the first one
    }
    
    CFRelease(list);
    CFRelease(power_info);
    
    printf("Battery keys test completed!\n");
    return 0;
}
