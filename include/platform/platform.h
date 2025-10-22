#ifndef PLATFORM_H
#define PLATFORM_H

#include <sys/utsname.h>
#include <string.h>

typedef enum {
    PLATFORM_UNKNOWN = 0,
    PLATFORM_MACOS,
    PLATFORM_LINUX,
    PLATFORM_BSD
} platform_t;

// Определение операционной системы
static inline platform_t detect_platform(void) {
    struct utsname uname_data;
    if (uname(&uname_data) != 0) {
        return PLATFORM_UNKNOWN;
    }
    
    // Проверяем sysname для определения ОС
    if (strcmp(uname_data.sysname, "Darwin") == 0) {
        return PLATFORM_MACOS;
    } else if (strcmp(uname_data.sysname, "Linux") == 0) {
        return PLATFORM_LINUX;
    } else if (strstr(uname_data.sysname, "BSD") != NULL) {
        return PLATFORM_BSD;
    }
    
    return PLATFORM_UNKNOWN;
}

// Получение строкового названия платформы
static inline const char* platform_name(platform_t platform) {
    switch (platform) {
        case PLATFORM_MACOS: return "macOS";
        case PLATFORM_LINUX: return "Linux";
        case PLATFORM_BSD: return "BSD";
        default: return "Unknown";
    }
}

// Проверка, является ли система Linux
static inline int is_linux(void) {
    return detect_platform() == PLATFORM_LINUX;
}

// Проверка, является ли система macOS
static inline int is_macos(void) {
    return detect_platform() == PLATFORM_MACOS;
}

#endif // PLATFORM_H
