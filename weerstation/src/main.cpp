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

//ntp servers
static const char *const timeServers[] = {
	"216.239.35.0",	 // time.google.com 
	"162.159.200.1", // time.cloudflare.com 
};
#define CLOCK_SYNC_QUERY_TIMEOUT_MS 5000
#define CLOCK_SYNC_RETRY_DELAY_SEC 30


static void sync_clock(void)
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
				return;
			}
		}
		server = (server + 1) % ARRAY_SIZE(timeServers);
		k_sleep(K_SECONDS(CLOCK_SYNC_RETRY_DELAY_SEC));
	}
}

int main(void)
{
	SensorManager sensorManager;
	static Storage storage;
	WebServer webServer;

	wifi_sta_connect();
	webServer.start(&storage);
	sync_clock(); // blocks until clock is synced to prevent incorrect timestamps. 
	while (true)
	{
		measurements m = sensorManager.get_measurements();
		storage.write_storage(&m);

		// just for testing
		static std::vector<measurements> measurements;
		measurements.clear();
		storage.read_last_n(measurements, 5);
		for (const auto &measurement : measurements)
		{
			printk("Temperature: %f, Pressure: %f, Humidity: %f\n",
				   measurement.temperature, measurement.pressure, measurement.humidity);
		}
		printk("\n");
		k_sleep(K_SECONDS(60));
	}
	return 0;
}
