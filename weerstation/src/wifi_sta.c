#include "wifi_sta.h"
#include "status_led.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#define HAS_PSK (sizeof(CONFIG_WIFI_PSK) > 1)

static struct net_mgmt_event_callback wifi_cb;

static void wifi_event(struct net_mgmt_event_callback *cb, uint64_t event,
                       struct net_if *iface)
{
    const struct wifi_status *status = cb->info;

    ARG_UNUSED(iface);

    if (event == NET_EVENT_WIFI_CONNECT_RESULT) {
        status_set_wifi(status != NULL &&
                        status->conn_status == WIFI_STATUS_CONN_SUCCESS
                        ? WIFI_CONNECTED
                        : WIFI_ERROR);
    } else if (event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        status_set_wifi(WIFI_DOWN);
    }
}

static void disable_power_save(struct net_if *iface)
{
    struct wifi_ps_params params = {
        .enabled = WIFI_PS_DISABLED,
        .type = WIFI_PS_PARAM_STATE,
    };
    int rc = net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params));

    printk("Wi-Fi power save %s (%d)\n", rc == 0 ? "disabled" : "not disabled", rc);
}

int wifi_sta_connect(void)
{
    struct net_if *iface = net_if_get_wifi_sta();

    struct wifi_connect_req_params params = {
        .ssid = (const uint8_t *)CONFIG_WIFI_SSID,
        .ssid_length = sizeof(CONFIG_WIFI_SSID) - 1,
        .psk = HAS_PSK ? (const uint8_t *)CONFIG_WIFI_PSK : NULL,
        .psk_length = HAS_PSK ? sizeof(CONFIG_WIFI_PSK) - 1 : 0,
        .security = HAS_PSK ? WIFI_SECURITY_TYPE_PSK
                            : WIFI_SECURITY_TYPE_NONE,
        .channel = WIFI_CHANNEL_ANY,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
    };
    int rc;

    if (iface == NULL)
    {
        printk("no Wi-Fi station interface\n");
        status_set_wifi(WIFI_ERROR);
        return -1;
    }

    if (wifi_cb.handler == NULL)
    {
        net_mgmt_init_event_callback(&wifi_cb, wifi_event,
                         NET_EVENT_WIFI_CONNECT_RESULT |
                         NET_EVENT_WIFI_DISCONNECT_RESULT);
        net_mgmt_add_event_callback(&wifi_cb);
    }

    status_set_wifi(WIFI_CONNECTING);
    rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    printk("Wi-Fi connect request -> %d\n", rc);

    if (rc == 0)
    {
        disable_power_save(iface);
    }
    else
    {
        status_set_wifi(WIFI_ERROR);
    }
    return rc;
}
