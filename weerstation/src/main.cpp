/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

const struct device *const sensortp = DEVICE_DT_GET(DT_ALIAS(sensortp));
const struct device *const sensorth = DEVICE_DT_GET(DT_ALIAS(sensorth));

static int counter = 0;

float read_sensor(const struct device *sensor, enum sensor_channel channel)
{
	sensor_sample_fetch(sensor);
	struct sensor_value value;
	sensor_channel_get(sensor, channel, &value);
	return sensor_value_to_float(&value);
}

int main(void)
{
	// check if the sensor is ready
	if (!device_is_ready(sensortp))
	{
		printk("Sensor device not ready\n");
		return -1;
	}

	while (true)
	{	
		float temperature = read_sensor(sensortp, SENSOR_CHAN_AMBIENT_TEMP);
		float pressure = read_sensor(sensortp, SENSOR_CHAN_PRESS);
		float humidity = read_sensor(sensorth, SENSOR_CHAN_HUMIDITY);
		float temp2 = read_sensor(sensorth, SENSOR_CHAN_AMBIENT_TEMP);
		float average_temp = (temperature + temp2) / 2.0;
		printk("average temperature: %f\n", average_temp);
		printk("pressure: %f\n", pressure);
		printk("humidity: %f\n", humidity);

		counter++;
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
