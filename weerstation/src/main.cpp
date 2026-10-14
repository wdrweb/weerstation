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

static int sync_clock()
{
	int rc = net_mgmt_event_wait(NET_EVENT_L4_CONNECTED, NULL, NULL, NULL, NULL,
				     K_SECONDS(30));
	if (rc)
	{
		printk("Waiting for network connection failed: %d\n", rc);
		return rc;
	}

	struct sntp_time sntp_time;
	rc = sntp_simple("pool.ntp.org", 5000, &sntp_time);
	if (rc)
	{
		printk("SNTP request failed: %d\n", rc);
		return rc;
	}

	struct timespec time = {
		.tv_sec = (time_t)sntp_time.seconds,
		.tv_nsec = (long)(((uint64_t)sntp_time.fraction * 1000000000ULL) >> 32),
	};
	rc = sys_clock_settime(SYS_CLOCK_REALTIME, &time);
	if (rc)
	{
		printk("clock not synced\n");
		return rc;
	}

	printk("clock synced\n");
	return 0;
}

int main(void)
{
	SensorManager sensorManager;
	static Storage storage;
	WebServer webServer;

	if (wifi_sta_connect() == 0)
	{
		sync_clock();
	}
	webServer.start(&storage);

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
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
