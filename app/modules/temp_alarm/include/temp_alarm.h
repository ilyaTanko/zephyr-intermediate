/*
 * Temperature Alarm Module - Public API
 *
 * Reads the ambient temperature from a Zephyr sensor device and raises an
 * alarm when a reading goes above a configurable threshold. The sensor is
 * reached through the sensor driver API, so unit tests replace it with a fake
 * struct device whose API table points at FFF fakes.
 */

#ifndef TEMP_ALARM_H
#define TEMP_ALARM_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Lowest accepted alarm threshold, in degrees Celsius. */
#define TEMP_ALARM_THRESHOLD_MIN 0
/** Highest accepted alarm threshold, in degrees Celsius. */
#define TEMP_ALARM_THRESHOLD_MAX 100

/** Snapshot of the alarm state, filled in by temp_alarm_get_status(). */
struct temp_alarm_status {
	/** Last temperature read successfully, in whole degrees Celsius. */
	int last_temp;
	/** Number of checks that found the temperature above the threshold. */
	uint32_t alarm_count;
	/** True if the last successful check was above the threshold. */
	bool is_alarming;
	/** Last error from the sensor driver since init or reset, or 0. */
	int last_error;
};

/**
 * @brief Initialise the alarm module.
 *
 * Clears all alarm state, so calling it again fully resets the module.
 *
 * @param sensor_dev Sensor device used for temperature readings.
 * @param threshold  Alarm threshold in degrees Celsius, in
 *                   [TEMP_ALARM_THRESHOLD_MIN, TEMP_ALARM_THRESHOLD_MAX].
 *
 * @return 0 on success,
 *         -ENODEV if @p sensor_dev is NULL or not ready,
 *         -EINVAL if @p threshold is out of range. On error the module
 *         state is left unchanged.
 */
int temp_alarm_init(const struct device *sensor_dev, int threshold);

/**
 * @brief Change the alarm threshold.
 *
 * Also clears is_alarming: the next temp_alarm_check() decides it again.
 *
 * @param threshold New threshold in degrees Celsius, in
 *                  [TEMP_ALARM_THRESHOLD_MIN, TEMP_ALARM_THRESHOLD_MAX].
 *
 * @return 0 on success,
 *         -EINVAL if @p threshold is out of range. The old threshold is kept.
 */
int temp_alarm_set_threshold(int threshold);

/**
 * @brief Read the sensor once and update the alarm state.
 *
 * Calls sensor_sample_fetch_chan() and then sensor_channel_get(), both for
 * SENSOR_CHAN_AMBIENT_TEMP. A reading strictly above the threshold raises the
 * alarm and increments alarm_count. Any other reading clears the alarm.
 * temp_alarm_init() must have succeeded first.
 *
 * @return 0 on success,
 *         any negative errno from sensor_sample_fetch_chan(), in which case
 *         sensor_channel_get() is not called,
 *         any negative errno from sensor_channel_get().
 *         On error the code is also stored in last_error.
 */
int temp_alarm_check(void);

/**
 * @brief Clear the alarm: is_alarming, alarm_count and last_error.
 *
 * The threshold and last_temp are kept.
 */
void temp_alarm_reset(void);

/**
 * @brief Copy the current alarm state into @p status.
 *
 * @param status Where to write the snapshot.
 *
 * @return 0 on success,
 *         -EINVAL if @p status is NULL.
 */
int temp_alarm_get_status(struct temp_alarm_status *status);

#ifdef __cplusplus
}
#endif

#endif /* TEMP_ALARM_H */
