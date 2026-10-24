#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum wifi_state {
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_DOWN,
    WIFI_ERROR,
};

enum station_state {
    STATION_BOOT,
    STATION_SERVING,
    STATION_ERROR,
};

void status_set_wifi(enum wifi_state state);
void status_set_station(enum station_state state);

#ifdef __cplusplus
}
#endif
