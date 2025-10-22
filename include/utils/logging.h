// logging.h
#ifndef LOGGING_H
#define LOGGING_H

void log_info(const char *msg);
void log_error(const char *msg);

// Append one CSV metrics line; returns 0 on success
int log_metrics_csv(const char *path,
                    float cpu_percent,
                    float mem_percent,
                    long uptime_sec,
                    float net_rx_bps,
                    float net_tx_bps);

#endif
