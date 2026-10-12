/*
 * temp_alarm - Error-path unit tests (homework skeleton)
 *
 * Task 2: fill in the stubs according to TEST_SPEC.md.
 * Each stub calls ztest_test_skip(), so the binary builds and runs cleanly until the test is implemented.
 *
 * Run:
 *   west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "sensor_fake.h"
#include "temp_alarm.h"

#define DEFAULT_THRESHOLD 30

/* Same before() hook as in test_basic.c: a clean fake and an initialised module. */
static void before(void* fixture) {
	ARG_UNUSED(fixture);

	sensor_fake_reset();
	sensor_fake_init();

	zassume_ok(temp_alarm_init(&fake_sensor_dev, DEFAULT_THRESHOLD), "precondition: temp_alarm_init must succeed");
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_sensor_errors
 *
 * Failures from the sensor driver, injected through the fakes.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_sensor_errors, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_sensor_errors, test_fetch_error_propagates) {
	fake_sensor_sample_fetch_fake.return_val = -EIO;

	zassert_equal(temp_alarm_check(), -EIO, "fetch error must be propagated");

	struct temp_alarm_status status;
	zassert_ok(temp_alarm_get_status(&status), "temp_alarm_get_status must succeed");

	zexpect_equal(status.last_error, -EIO, "last_error must contain the fetch error, got %d", status.last_error);
	zexpect_equal(fake_sensor_channel_get_fake.call_count, 0U, "channel_get must not be called after a failed fetch, got %u calls", fake_sensor_channel_get_fake.call_count);
}

ZTEST(temp_alarm_sensor_errors, test_channel_get_error_propagates) {
	// Precondition: channel_get returns -EIO
	// input : 	temp_alarm_check() - expected : 	- EIO
	// Verify #2: status.last_error == -EIO, and the sample was still fetched exactly once. Note that sensor_fake_init() has already configured channel_get in the before hook.
	fake_sensor_channel_get_fake.custom_fake = NULL;
	fake_sensor_channel_get_fake.return_val = -EIO;

	zassert_equal(temp_alarm_check(), -EIO, "channel_get error must be propagated");

	struct temp_alarm_status status;
	zassert_ok(temp_alarm_get_status(&status), "temp_alarm_get_status must succeed");

	zexpect_equal(status.last_error, -EIO, "last_error must contain the channel_get error, got %d", status.last_error);
	zexpect_equal(fake_sensor_sample_fetch_fake.call_count, 1U, "the sample must be fetched exactly once, got %u calls", fake_sensor_sample_fetch_fake.call_count);
}

ZTEST(temp_alarm_sensor_errors, test_fail_then_recover) {
	int fetch_results[] = {-EIO, 0};
	SET_RETURN_SEQ(fake_sensor_sample_fetch, fetch_results, ARRAY_SIZE(fetch_results));
	sensor_fake_set_temperature(20);

	zassert_equal(temp_alarm_check(), -EIO, "the first check must report the fetch error");
	zassert_ok(temp_alarm_check(), "the second check must recover");

	struct temp_alarm_status status;
	zassert_ok(temp_alarm_get_status(&status), "temp_alarm_get_status must succeed");

	zexpect_equal(status.last_temp, 20, "last_temp must be 20 after recovery, got %d", status.last_temp);
	zexpect_false(status.is_alarming, "20 C must not raise the alarm");
	zexpect_equal(status.last_error, -EIO, "last_error must retain the earlier error, got %d", status.last_error);
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_invalid_input
 *
 * Arguments the public API must reject.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_invalid_input, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_invalid_input, test_get_status_null) {
	zassert_equal(temp_alarm_get_status(NULL), -EINVAL, "a NULL status pointer must be rejected");
}

ZTEST(temp_alarm_invalid_input, test_init_without_device) {
	zassert_equal(temp_alarm_init(NULL, DEFAULT_THRESHOLD), -ENODEV, "a NULL sensor device must be rejected");
}

ZTEST(temp_alarm_invalid_input, test_init_threshold_out_of_range) {
	zassert_equal(temp_alarm_init(&fake_sensor_dev, -1), -EINVAL, "a threshold below the minimum must be rejected");
	zassert_equal(temp_alarm_init(&fake_sensor_dev, 101), -EINVAL, "a threshold above the maximum must be rejected");
}

ZTEST(temp_alarm_invalid_input, test_set_threshold_out_of_range) {
	zassert_equal(temp_alarm_set_threshold(-1), -EINVAL, "a threshold below the minimum must be rejected");
	zassert_equal(temp_alarm_set_threshold(101), -EINVAL, "a threshold above the maximum must be rejected");

	sensor_fake_set_temperature(40);
	zassert_ok(temp_alarm_check(), "temp_alarm_check must succeed");

	struct temp_alarm_status status;
	zassert_ok(temp_alarm_get_status(&status), "temp_alarm_get_status must succeed");
	zexpect_true(status.is_alarming, "40 C must still exceed the unchanged 30 C threshold");
}
