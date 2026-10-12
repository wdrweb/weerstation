#include <vector>
#include <zephyr/kernel.h>
#include "storage.h"
#include "sensor_manager.h"

int main(void)
{
	SensorManager sensorManager;
	static Storage storage;

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
