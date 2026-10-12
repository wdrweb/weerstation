#include "sensor_manager.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>

const struct device *const sensortp = DEVICE_DT_GET(DT_ALIAS(sensortp));
const struct device *const sensorth = DEVICE_DT_GET(DT_ALIAS(sensorth));

SensorManager::SensorManager()
{
    // check if the sensor is ready
    if (!device_is_ready(sensortp))
    {
        printk("Sensor device not ready\n");
        return;
    }
    if (!device_is_ready(sensorth))
    {
        printk("Sensor device not ready\n");
        return;
    }
}

SensorManager::~SensorManager()
{
}

void SensorManager::fetch_sample(const struct device *sensor)
{
    int rc;
    rc = sensor_sample_fetch(sensor);
    if (rc)
    {
        printk("sensor_sample_fetch failed: %d\n", rc);
    }
}
float SensorManager::get_channel(const struct device *sensor, enum sensor_channel channel)
{
    struct sensor_value value;
    int rc = sensor_channel_get(sensor, channel, &value);
    if (rc)
    {
        printk("sensor_channel_get failed: %d\n", rc);
    }
    return sensor_value_to_float(&value);
}

measurements SensorManager::get_measurements()
{
    measurements m;
    this->fetch_sample(sensortp);
    this->fetch_sample(sensorth);
    float temperature1 = this->get_channel(sensortp, SENSOR_CHAN_AMBIENT_TEMP);
    float temperature2 = this->get_channel(sensorth, SENSOR_CHAN_AMBIENT_TEMP);
    m.pressure = this->get_channel(sensortp, SENSOR_CHAN_PRESS);
    m.humidity = this->get_channel(sensorth, SENSOR_CHAN_HUMIDITY);
    m.temperature = (temperature1 + temperature2) / 2.0f;
    return m;
}
