/*
 * Sensor Fake - definitions, simulated temperature and the fake device
 */

#include "sensor_fake.h"

#include <zephyr/fff.h>
#include <zephyr/toolchain.h>

/* Exactly one of these per test binary. */
DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, fake_sensor_channel_get, const struct device*, enum sensor_channel, struct sensor_value*);
DEFINE_FAKE_VALUE_FUNC(int, fake_sensor_sample_fetch, const struct device*, enum sensor_channel);

/* ---- Simulated sensor state ---------------------------------------------- */

static int sim_temperature;

/*
 * Writes the simulated temperature into the caller's sensor_value. val1 holds
 * the whole degrees; val2 (millionths) is not used, so it is set to 0.
 */
static int custom_sensor_channel_get(const struct device* dev, enum sensor_channel chan, struct sensor_value* val) {
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	val->val1 = sim_temperature;
	val->val2 = 0;

	return 0;
}

/* ---- Fake sensor device -------------------------------------------------- */

/* The API table temp_alarm.c calls through. */
static const struct sensor_driver_api fake_sensor_api = {
	.sample_fetch = fake_sensor_sample_fetch,
	.channel_get = fake_sensor_channel_get,
};

/* initialized = 1 and init_res = 0 make device_is_ready() return true. */
static struct device_state fake_sensor_state = {
	.init_res = 0,
	.initialized = 1,
};

struct device fake_sensor_dev = {
	.name = "fake_sensor",
	.api = &fake_sensor_api,
	.state = &fake_sensor_state,
};

/* ---- Public helpers ------------------------------------------------------ */

void sensor_fake_set_temperature(int temp_celsius) {
	sim_temperature = temp_celsius;
}

void sensor_fake_init(void) {
	fake_sensor_channel_get_fake.custom_fake = custom_sensor_channel_get;
}

void sensor_fake_reset(void) {
	RESET_FAKE(fake_sensor_channel_get);
	RESET_FAKE(fake_sensor_sample_fetch);
	FFF_RESET_HISTORY();

	sim_temperature = 0;
}
