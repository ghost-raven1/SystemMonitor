#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include <stddef.h>
#include <time.h>

// Helper function to get total memory in bytes
long get_total_memory(void);

// clipped print helper to keep lines within terminal width and avoid wrapping
void mvprintw_clip(int y, int x, const char *fmt, ...);

// colored clipped print
void mvprintw_clip_color(int y, int x, int color_pair, const char *fmt, ...);

// draw horizontal separator line across terminal width starting at x
void draw_hsep(int y, int x);

// render a simple colored bar for percent [0..100]
void render_bar(int y, int x, int width, float percent, int colors_on);

// pick color pair by percent thresholds (green/yellow/red)
int choose_color_by_percent(float percent, float warn, float crit);

const char *resolve_config_path();

// cache Wi‑Fi SSID for 60s (macOS)
void fetch_ssid_cached(char *out, size_t out_sz);

void format_uptime(long seconds, char *out, size_t out_sz);

// simple weather fetcher using wttr.in; cached to avoid frequent network calls
void fetch_city_cached(char *out, size_t out_sz);

void fetch_weather_cached(char *out, size_t out_sz);

#endif // UI_HELPERS_H