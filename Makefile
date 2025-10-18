# Makefile

CC = gcc
CFLAGS = -Wall -O2 -Iinclude
LDFLAGS = -lncurses -framework CoreFoundation -framework IOKit

SRC = src/main.c \
      src/ui.c \
      src/system_info.c \
      src/processes.c \
      src/config.c \
      src/disk.c \
      src/grafana.c \
      src/logging.c \
      src/mac_smc.c \
      src/network.c \
      src/notifications.c \
      src/prometheus.c \
      src/battery.c \
      src/usb.c \
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
