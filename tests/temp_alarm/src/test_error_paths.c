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
static void before(void *fixture) {
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

ZTEST(temp_alarm_sensor_errors, test_fetch_error_propagates)
{
	/* TODO(l8-task2): a failed fetch is reported,
	 * and the module does not go on to read the channel.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #1.
	 */
	ztest_test_skip();
}

ZTEST(temp_alarm_sensor_errors, test_channel_get_error_propagates)
{
	/* TODO(l8-task2): a failed channel read is reported.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #2.
	 */
	ztest_test_skip();
}

ZTEST(temp_alarm_sensor_errors, test_fail_then_recover)
{
	/* TODO(l8-task2): the first fetch fails and the second one succeeds.
	 * The module must recover on the second check.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #3.
	 */
	ztest_test_skip();
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_invalid_input
 *
 * Arguments the public API must reject.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_invalid_input, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_invalid_input, test_get_status_null)
{
	/* TODO(l8-task2): a NULL status pointer is rejected.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #1.
	 */
	ztest_test_skip();
}

ZTEST(temp_alarm_invalid_input, test_init_without_device)
{
	/* TODO(l8-task2): init without a sensor device is rejected.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #2.
	 */
	ztest_test_skip();
}

ZTEST(temp_alarm_invalid_input, test_init_threshold_out_of_range)
{
	/* TODO(l8-task2): init rejects thresholds outside the allowed range.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #3.
	 */
	ztest_test_skip();
}

ZTEST(temp_alarm_invalid_input, test_set_threshold_out_of_range)
{
	/* TODO(l8-task2): set_threshold rejects values outside the allowed range,
	 * and keeps the old threshold.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #4.
	 */
	ztest_test_skip();
}
