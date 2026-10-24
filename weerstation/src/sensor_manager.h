#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "measurements.h"

extern const struct device *const sensortp;
extern const struct device *const sensorth;

class SensorManager
{
public:
	SensorManager();
	~SensorManager();
	//float read_sensor(const struct device *sensor, enum sensor_channel channel);
	void fetch_sample(const struct device *sensor);
	float get_channel(const struct device *sensor, enum sensor_channel channel);
	measurements get_measurements();
	bool is_ready() const { return ready; }
	

private:
	bool ready = true;
};