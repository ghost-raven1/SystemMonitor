#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>

int main() {
    printf("Testing CoreFoundation power sources...\n");
    
    printf("About to call IOPSCopyPowerSourcesInfo...\n");
    CFTypeRef power_info = IOPSCopyPowerSourcesInfo();
    if (!power_info) {
        printf("IOPSCopyPowerSourcesInfo returned NULL\n");
        return 0;
    }
    printf("IOPSCopyPowerSourcesInfo succeeded\n");
    
    printf("About to call IOPSCopyPowerSourcesList...\n");
    CFArrayRef list = IOPSCopyPowerSourcesList(power_info);
    if (!list) {
        printf("IOPSCopyPowerSourcesList returned NULL\n");
        CFRelease(power_info);
        return 0;
    }
    printf("IOPSCopyPowerSourcesList succeeded\n");
    
    CFIndex count = CFArrayGetCount(list);
    printf("Found %ld power sources\n", count);
    
    CFRelease(list);
    CFRelease(power_info);
    
    printf("CoreFoundation test completed successfully!\n");
    return 0;
}









