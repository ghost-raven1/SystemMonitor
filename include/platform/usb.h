#ifndef USB_H
#define USB_H

typedef struct {
	char name[64];
	char type[32];
	float speed_mbps; // может быть 0, если неизвестно
} usb_device_t;

int list_usb_devices(usb_device_t *out, int max, int *count);

#endif

