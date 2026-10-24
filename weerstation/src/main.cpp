#include <vector>
#include <zephyr/kernel.h>
#include <zephyr/net/net_event.h>
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
	"216.239.35.0",	 // time.google.com 
	"162.159.200.1", // time.cloudflare.com 
};
#define CLOCK_SYNC_QUERY_TIMEOUT_MS 5000
#define CLOCK_SYNC_RETRY_DELAY_SEC 30


static void sync_clock(bool station_ok)
{
	struct sntp_time server_time;
	size_t server = 0;
	// wait for network connection 
	net_mgmt_event_wait(NET_EVENT_IPV4_DHCP_BOUND, NULL, NULL, NULL, NULL,
				K_SECONDS(30));
	
	// keep trying to sync, network may not be ready
	while (true)
	{
		int rc = sntp_simple(timeServers[server], CLOCK_SYNC_QUERY_TIMEOUT_MS,
				     &server_time);
		if (rc == 0)
		{
			struct timespec time = {
				.tv_sec = (time_t)server_time.seconds,
				.tv_nsec = (long)(((uint64_t)server_time.fraction * 1000000000ULL) >> 32),
			};
			if (sys_clock_settime(SYS_CLOCK_REALTIME, &time) == 0)
			{
				status_set_station(station_ok ? STATION_SERVING : STATION_ERROR);
				return;
			}
		}
		status_set_station(STATION_ERROR);
		server = (server + 1) % ARRAY_SIZE(timeServers);
		k_sleep(K_SECONDS(CLOCK_SYNC_RETRY_DELAY_SEC));
	}
}


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

	sync_clock(ok); // blocks until clock is synced to prevent incorrect timestamps.
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
