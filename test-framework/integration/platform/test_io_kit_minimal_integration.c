#include <stdio.h>
#include <stdlib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <CoreFoundation/CoreFoundation.h>

int main() {
    printf("Starting IOKit test...\n");
    
    printf("Calling IOPSCopyPowerSourcesInfo...\n");
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        printf("IOPSCopyPowerSourcesInfo returned NULL\n");
        return 1;
    }
    printf("IOPSCopyPowerSourcesInfo succeeded\n");
    
    printf("Calling IOPSCopyPowerSourcesList...\n");
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) {
        printf("IOPSCopyPowerSourcesList returned NULL\n");
        CFRelease(power_info);
        return 1;
    }
    printf("IOPSCopyPowerSourcesList succeeded\n");
    
    printf("Getting array count...\n");
    CFIndex count = CFArrayGetCount(list);
    printf("Array count: %ld\n", count);
    
    printf("Releasing resources...\n");
    CFRelease(list);
    CFRelease(power_info);
    
    printf("IOKit test completed successfully\n");
    return 0;
}
