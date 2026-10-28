#include <string.h>
#include <vector>
#include <zephyr/kernel.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/clock.h>
#include <time.h>
#include "storage.h"
#include "sensor_manager.h"
#include "wifi_sta.h"
#include "web_server.h"
#include "status_led.h"
#include <zephyr/sys/clock.h>


//ntp servers
static const char *const timeServers[] = {
	"nl.pool.ntp.org",
	"pool.ntp.org",
};
#define CLOCK_SYNC_QUERY_TIMEOUT_MS 5000
#define CLOCK_SYNC_RETRY_DELAY_MS 30000
// The clock is derived from uptime and drifts, so refresh it now and then
// instead of trusting one sync for the whole uptime.
#define CLOCK_RESYNC_INTERVAL_SEC (6 * 60 * 60)


static void log_ip_address(void)
{
	static char shown[NET_IPV4_ADDR_LEN];
	struct net_in_addr *addr =
		net_if_ipv4_get_global_addr(net_if_get_default(), NET_ADDR_PREFERRED);
	char buf[NET_IPV4_ADDR_LEN];

	if (addr == NULL || net_addr_ntop(AF_INET, addr, buf, sizeof(buf)) == NULL) {
		return;
	}

	if (strcmp(shown, buf) != 0) {
		printk("IP address: %s\n", buf);
		strcpy(shown, buf);
	}
}

static void sync_clock_thread(void *, void *, void *)
{
	struct sntp_time server_time;
	size_t server = 0;

	while (true)
	{
		// Retry after the delay, but wake up early when a late hotspot hands
		// out a lease so the samples get absolute timestamps sooner.
		net_mgmt_event_wait(NET_EVENT_IPV4_DHCP_BOUND, NULL, NULL, NULL, NULL,
				    K_MSEC(CLOCK_SYNC_RETRY_DELAY_MS));
		log_ip_address();

		int rc = sntp_simple(timeServers[server], CLOCK_SYNC_QUERY_TIMEOUT_MS,
				     &server_time);
		if (rc == 0)
		{
			struct timespec time = {
				.tv_sec = (time_t)server_time.seconds,
				.tv_nsec = (long)(((uint64_t)server_time.fraction * 1000000000ULL) >> 32),
			};
			int set_rc = sys_clock_settime(SYS_CLOCK_REALTIME, &time);

			if (set_rc == 0)
			{
				printk("Clock synced from %s\n", timeServers[server]);
				server = 0;
				k_sleep(K_SECONDS(CLOCK_RESYNC_INTERVAL_SEC));
				continue;
			}

			printk("sys_clock_settime failed (%d)\n", set_rc);
		}
		else
		{
			printk("Clock sync with %s failed (%d)\n", timeServers[server], rc);
		}
		server = (server + 1) % ARRAY_SIZE(timeServers);
	}
}

//sntp_simple resolves a hostname, so the stack needs resolver room
K_THREAD_STACK_DEFINE(clock_sync_stack, 3072);
static struct k_thread clock_sync_thread_data;

static int64_t ms_to_next_minute(void)
{
	struct timespec ts;
	sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	int64_t ms = (ts.tv_sec % 60) * 1000 + ts.tv_nsec / 1000000;
	return ms == 0 ? 0 : 60000 - ms;
}

int main(void)
{
	SensorManager sensorManager;
	static Storage storage;
	WebServer webServer;

	wifi_sta_connect();
	int rc = webServer.start(&storage);
	bool ok = sensorManager.is_ready() && rc == 0;
	status_set_station(ok ? STATION_SERVING : STATION_ERROR);

	//sync runs in the background; samples are flagged relative until it succeeds
	k_thread_create(&clock_sync_thread_data, clock_sync_stack,
			K_THREAD_STACK_SIZEOF(clock_sync_stack),
			sync_clock_thread, NULL, NULL, NULL, 7, 0, K_NO_WAIT);

	static struct k_timer ticker;
	k_timer_init(&ticker, NULL, NULL);
	k_timer_start(&ticker, K_MSEC(ms_to_next_minute()), K_SECONDS(60));
	while (true)
	{
		k_timer_status_sync(&ticker);
		measurements m = sensorManager.get_measurements();
		ok = storage.write_storage(&m) == 0 && sensorManager.is_ready();
		status_set_station(ok ? STATION_SERVING : STATION_ERROR);
	}
	return 0;
}
