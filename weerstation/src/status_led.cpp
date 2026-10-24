#include "status_led.h"

#include <string.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>

#define BLINK_MS 500

static const struct device *const strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static atomic_t wifi_state = ATOMIC_INIT(WIFI_IDLE);
static atomic_t station_state = ATOMIC_INIT(STATION_BOOT);

void status_set_wifi(enum wifi_state state)
{
    if (state == WIFI_DOWN && atomic_get(&wifi_state) == WIFI_ERROR) {
        return;
    }
    atomic_set(&wifi_state, state);
}

void status_set_station(enum station_state state)
{
    atomic_set(&station_state, state);
}

static struct led_rgb rgb(uint8_t r, uint8_t g, uint8_t b)
{
    struct led_rgb c = {};

    c.r = r;
    c.g = g;
    c.b = b;
    return c;
}

static struct led_rgb wanted(void)
{
    int station = (int)atomic_get(&station_state);
    int wifi = (int)atomic_get(&wifi_state);
    bool lit = k_uptime_get() / BLINK_MS % 2 == 0;

    if (station == STATION_ERROR || wifi == WIFI_ERROR) {
        return rgb(0x30, 0, 0);
    }
    if (wifi == WIFI_CONNECTING) {
        return rgb(lit ? 0x30 : 0, lit ? 0x18 : 0, 0);
    }
    if (wifi == WIFI_DOWN) {
        return rgb(lit ? 0x30 : 0, lit ? 0x08 : 0, 0);
    }
    if (station == STATION_BOOT) {
        return rgb(0x10, 0x10, 0x10);
    }
    if (station == STATION_SERVING) {
        return wifi == WIFI_CONNECTED ? rgb(0, 0x20, 0) : rgb(0, 0, 0x20);
    }
    return rgb(0x20, 0, 0x20);
}

static void status_led_thread(void *, void *, void *)
{
    const bool ready = device_is_ready(strip);
    static struct led_rgb shown;
    struct led_rgb next;

    if (!ready) {
        printk("LED strip not ready\n");
    }
    while (true) {
        next = wanted();
        if (ready && memcmp(&next, &shown, sizeof(next)) != 0) {
            led_strip_update_rgb(strip, &next, 1);
            shown = next;
        }
        k_sleep(K_MSEC(BLINK_MS / 4));
    }
}

K_THREAD_DEFINE(status_led_tid, 1024, status_led_thread, NULL, NULL, NULL, 7, 0, 0);
