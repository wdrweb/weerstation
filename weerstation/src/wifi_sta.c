#include "wifi_sta.h"

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#define HAS_PSK (sizeof(CONFIG_WIFI_PSK) > 1)

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

	if (iface == NULL) {
		printk("no Wi-Fi station interface\n");
		return -1;
	}

	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
	printk("Wi-Fi connect request -> %d\n", rc);

	return rc;
}
