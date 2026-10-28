#include "sensor_manager.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/sys/clock.h>
#include <time.h>

const struct device *const sensortp = DEVICE_DT_GET(DT_ALIAS(sensortp));
const struct device *const sensorth = DEVICE_DT_GET(DT_ALIAS(sensorth));

SensorManager::SensorManager()
{
    ready = device_is_ready(sensortp) && device_is_ready(sensorth);
    if (!ready)
    {
        printk("Sensor device not ready\n");
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

//before SNTP runs the realtime offset is zero, so this is just uptime
static uint64_t clock_seconds(bool *relative)
{
    struct timespec offset;
    struct timespec now;

    sys_clock_getrtoffset(&offset);
    *relative = offset.tv_sec == 0 && offset.tv_nsec == 0;

    if (sys_clock_gettime(SYS_CLOCK_REALTIME, &now) != 0)
    {
        *relative = true;
        return 0;
    }

    return (uint64_t)now.tv_sec;
}

measurements SensorManager::get_measurements()
{
    measurements m;
    this->fetch_sample(sensortp);
    this->fetch_sample(sensorth);
    m.pressure = this->get_channel(sensortp, SENSOR_CHAN_PRESS);
    m.humidity = this->get_channel(sensorth, SENSOR_CHAN_HUMIDITY);
    //bmp280 temp not used as it is inaccurate
    m.temperature = this->get_channel(sensorth, SENSOR_CHAN_AMBIENT_TEMP);
    bool relative;
    m.timestamp = clock_seconds(&relative);
    m.relative = relative ? 1 : 0;
    return m;
}
