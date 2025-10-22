// enhanced.h - Enhanced features for System Monitor
// Weather, system events, network connections, and more

#ifndef ENHANCED_H
#define ENHANCED_H

#include <time.h>

// Weather structures
typedef struct {
    char condition[16];
    const char *icon;
    const char *description;
} weather_icon_t;

typedef struct {
    float temperature;
    float feels_like;
    int humidity;
    int pressure;
    float wind_speed;
    int wind_dir;
    char condition[128];
    char city[64];
    char icon[8];
    time_t sunrise;
    time_t sunset;
    int valid;
} enhanced_weather_t;

typedef struct {
    char *data;
    size_t size;
} weather_response_t;

typedef struct {
    enhanced_weather_t data;
    time_t timestamp;
    int valid;
} weather_cache_t;

// Network connection structures
typedef struct {
    char protocol[16];
    char local_addr[128];
    char remote_addr[128];
    char state[32];
    char service[32];
    unsigned long bytes_sent;
    unsigned long bytes_recv;
    int pid;
} network_connection_t;

typedef struct {
    network_connection_t connections[100];
    int count;
    time_t timestamp;
    int valid;
} network_connections_cache_t;

// System events
typedef enum {
    EVENT_INFO,
    EVENT_WARNING,
    EVENT_ERROR,
    EVENT_CRITICAL
} event_severity_t;

typedef struct {
    time_t timestamp;
    event_severity_t severity;
    char process[64];
    char message[256];
    char icon[8];
} system_event_t;

typedef struct {
    system_event_t events[50];
    int count;
    time_t timestamp;
    int valid;
} system_events_cache_t;

// Docker structures
typedef struct {
    int running;
    char version[32];
    int containers_running;
    int containers_total;
    int images;
    int volumes;
    int networks;
} docker_status_t;

typedef struct {
    char id[16];
    char name[64];
    char image[128];
    char status[64];
    char ports[128];
    float cpu_percent;
    char memory_usage[32];
    int running;
} docker_container_t;

// Quick actions
typedef enum {
    ACTION_CLEAN_CACHE,
    ACTION_RESTART_NETWORK,
    ACTION_FLUSH_DNS,
    ACTION_FREE_MEMORY,
    ACTION_UPDATE_SYSTEM,
    ACTION_SHOW_LARGE_FILES,
    ACTION_KILL_DOCK,
    ACTION_KILL_FINDER,
    ACTION_TOGGLE_HIDDEN_FILES,
    ACTION_EMPTY_TRASH
} quick_action_t;

// System recommendations
typedef enum {
    REC_INFO,
    REC_WARNING,
    REC_CRITICAL
} recommendation_severity_t;

typedef struct {
    recommendation_severity_t severity;
    char icon[8];
    char title[128];
    char description[256];
    quick_action_t action;
} recommendation_t;

// Function declarations
int fetch_enhanced_weather(enhanced_weather_t *weather);
int get_network_connections(network_connection_t *conns, int max_conns, int *count);
int get_system_events(system_event_t *events, int max_events, int *count);
int check_docker_status(docker_status_t *status);
int get_docker_containers(docker_container_t *containers, int max_containers, int *count);
int execute_quick_action(quick_action_t action, char *output, size_t output_size);
int get_system_recommendations(recommendation_t *recs, int max_recs, int *count);

// Utility functions
void format_bytes(unsigned long long bytes, char *output, size_t size);
const char* get_wind_direction(int degrees);
void format_time(time_t timestamp, char *output, size_t size);

#endif // ENHANCED_H