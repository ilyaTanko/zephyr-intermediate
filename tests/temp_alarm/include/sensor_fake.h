/*
 * Sensor Fake - FFF fakes behind a fake struct device
 */

#ifndef SENSOR_FAKE_H
#define SENSOR_FAKE_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fff.h>

/* FFF fake for sensor_driver_api.channel_get. */
DECLARE_FAKE_VALUE_FUNC(int, fake_sensor_channel_get, const struct device*, enum sensor_channel, struct sensor_value*);
DECLARE_FAKE_VALUE_FUNC(int, fake_sensor_sample_fetch, const struct device*, enum sensor_channel);

/** The fake sensor device the tests pass to temp_alarm_init(). */
extern struct device fake_sensor_dev;

/** @brief Set the temperature, in whole degrees Celsius, that channel_get returns. */
void sensor_fake_set_temperature(int temp_celsius);

/**
 * @brief Install the custom_fake on fake_sensor_channel_get.
 *
 * Call it after sensor_fake_reset(), because RESET_FAKE() clears custom_fake.
 */
void sensor_fake_init(void);

/** @brief Reset every FFF fake, the call history and the simulated temperature. */
void sensor_fake_reset(void);

#endif /* SENSOR_FAKE_H */
