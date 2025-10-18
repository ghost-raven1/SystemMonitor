// notifications.c
#include "notifications.h"
#include <stdio.h>

void send_notification(const char *title, const char *message) {
    printf("[NOTIFY] %s: %s\n", title, message);
}
