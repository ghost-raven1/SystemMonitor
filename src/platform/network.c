// network.c
#include "platform/network.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

static unsigned long long prev_rx_bytes = 0;
static unsigned long long prev_tx_bytes = 0;
static struct timespec prev_ts = {0, 0};

static void read_total_bytes(unsigned long long *rx, unsigned long long *tx) {
	struct ifaddrs *ifap = NULL;
	*rx = 0;
	*tx = 0;
	if (getifaddrs(&ifap) != 0 || ifap == NULL) return;

	for (struct ifaddrs *ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) continue;
		if (ifa->ifa_addr->sa_family != AF_LINK) continue;
		if (!(ifa->ifa_flags & IFF_UP)) continue;
		if (ifa->ifa_data == NULL) continue;

		struct if_data *data = (struct if_data *)ifa->ifa_data;
		if (data) {
			*rx += data->ifi_ibytes;
			*tx += data->ifi_obytes;
		}
	}
	freeifaddrs(ifap);
}

static double elapsed_seconds_since(const struct timespec *start, const struct timespec *end) {
	return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) / 1e9;
}

static void ensure_baseline() {
	if (prev_ts.tv_sec == 0 && prev_ts.tv_nsec == 0) {
		read_total_bytes(&prev_rx_bytes, &prev_tx_bytes);
		clock_gettime(CLOCK_MONOTONIC, &prev_ts);
	}
}

float get_network_rx() {
	ensure_baseline();

	unsigned long long cur_rx = 0, cur_tx = 0;
	struct timespec now;
	read_total_bytes(&cur_rx, &cur_tx);
	clock_gettime(CLOCK_MONOTONIC, &now);

	double dt = elapsed_seconds_since(&prev_ts, &now);
	if (dt <= 0.0) return 0.0f;

	float rx_rate = (float)(cur_rx - prev_rx_bytes) / (float)dt; // bytes/s

	prev_rx_bytes = cur_rx;
	prev_tx_bytes = cur_tx;
	prev_ts = now;

	return rx_rate; // bytes per second
}

float get_network_tx() {
	ensure_baseline();

	unsigned long long cur_rx = 0, cur_tx = 0;
	struct timespec now;
	read_total_bytes(&cur_rx, &cur_tx);
	clock_gettime(CLOCK_MONOTONIC, &now);

	double dt = elapsed_seconds_since(&prev_ts, &now);
	if (dt <= 0.0) return 0.0f;

	float tx_rate = (float)(cur_tx - prev_tx_bytes) / (float)dt; // bytes/s

	prev_rx_bytes = cur_rx;
	prev_tx_bytes = cur_tx;
	prev_ts = now;

	return tx_rate; // bytes per second
}

const char *get_network_info() {
	static char buf[128];
    float rx_bps = get_network_rx();
    float tx_bps = get_network_tx();

    // Безопасный fallback - используем только системные вызовы без внешних команд
    // Если основные метрики недоступны, показываем нули вместо вызова внешних утилит

	float rx_kbps = rx_bps / 1024.0f;
	float tx_kbps = tx_bps / 1024.0f;
	snprintf(buf, sizeof(buf), "RX: %.2f KB/s  TX: %.2f KB/s", rx_kbps, tx_kbps);
	return buf;
}
