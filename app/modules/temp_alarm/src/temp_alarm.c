/*
 * Temperature Alarm Module - Implementation
 *
 * Both sensor calls in temp_alarm_check() are __syscall wrappers, so they are
 * inlined into this file and cannot be faked at link time. They end in a call
 * through the device's API table, which is where the unit tests intercept them.
 */

#include "temp_alarm.h"

#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(temp_alarm, LOG_LEVEL_INF);

/* Module-private state. temp_alarm_init() resets all of it. */
struct temp_alarm_data {
	int      threshold;
	int      last_temp;
	uint32_t alarm_count;
	bool     is_alarming;
	int      last_error;
};

static struct temp_alarm_data ta_data;
static const struct device *ctx_sensor_dev;

int temp_alarm_init(const struct device *sensor_dev, int threshold)
{
	if (!device_is_ready(sensor_dev)) {
		LOG_ERR("Sensor device is not ready");
		return -ENODEV;
	}

	if (threshold < TEMP_ALARM_THRESHOLD_MIN || threshold > TEMP_ALARM_THRESHOLD_MAX) {
		return -EINVAL;
	}

	ctx_sensor_dev      = sensor_dev;
	ta_data.threshold   = threshold;
	ta_data.last_temp   = 0;
	ta_data.alarm_count = 0U;
	ta_data.is_alarming = false;
	ta_data.last_error  = 0;

	LOG_INF("Temperature alarm initialized (threshold=%d C)", threshold);
	return 0;
}

int temp_alarm_set_threshold(int threshold)
{
	if (threshold < TEMP_ALARM_THRESHOLD_MIN || threshold > TEMP_ALARM_THRESHOLD_MAX) {
		return -EINVAL;
	}

	ta_data.threshold   = threshold;
	ta_data.is_alarming = false;

	LOG_INF("Threshold updated to %d C", threshold);
	return 0;
}

int temp_alarm_check(void)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch_chan(ctx_sensor_dev, SENSOR_CHAN_AMBIENT_TEMP);
	if (ret < 0) {
		ta_data.last_error = ret;
		LOG_ERR("Sensor fetch failed: %d", ret);
		return ret;
	}

	ret = sensor_channel_get(ctx_sensor_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
	if (ret < 0) {
		ta_data.last_error = ret;
		LOG_ERR("Sensor channel get failed: %d", ret);
		return ret;
	}

	int temp = val.val1;

	ta_data.last_temp = temp;

	if (temp > ta_data.threshold) {
		ta_data.is_alarming = true;
		ta_data.alarm_count++;
		LOG_WRN("Alarm! temp=%d > threshold=%d", temp, ta_data.threshold);
	} else {
		ta_data.is_alarming = false;
	}

	return 0;
}

void temp_alarm_reset(void)
{
	ta_data.is_alarming = false;
	ta_data.alarm_count = 0U;
	ta_data.last_error  = 0;

	LOG_INF("Alarm state reset");
}

int temp_alarm_get_status(struct temp_alarm_status *status)
{
	if (status == NULL) {
		return -EINVAL;
	}

	status->last_temp   = ta_data.last_temp;
	status->alarm_count = ta_data.alarm_count;
	status->is_alarming = ta_data.is_alarming;
	status->last_error  = ta_data.last_error;

	return 0;
}
