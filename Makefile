# Makefile

CC = gcc
CFLAGS = -Wall -O2 -Iinclude

# Определение операционной системы
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS = -lncurses -framework CoreFoundation -framework IOKit
    PLATFORM_SRC = src/mac_smc.c src/battery.c src/usb.c
else ifeq ($(UNAME_S),Linux)
    # Linux
    LDFLAGS = -lncurses
    PLATFORM_SRC = src/linux_hwmon.c src/battery_linux.c src/usb_linux.c
else
    # Fallback для других систем (BSD и т.д.)
    LDFLAGS = -lncurses
    PLATFORM_SRC = src/battery.c src/usb.c
endif

SRC = src/main.c \
      src/ui.c \
      src/system_info.c \
      src/processes.c \
      src/config.c \
      src/disk.c \
      src/grafana.c \
      src/logging.c \
      $(PLATFORM_SRC) \
      src/network.c \
      src/notifications.c \
      src/prometheus.c \
      src/gpu.c \
      src/anomalies.c \
      src/smart.c \
      src/developer_impl.c

OBJ = $(SRC:.c=.o)
TARGET = sysmon

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
