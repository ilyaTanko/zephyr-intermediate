/*
 * Calculator NVS Unit Tests - Happy Paths
 *
 * Run with:
 *   west twister -T tests/calculator_nvs -p qemu_x86 -O app/build/twister-out/calculator_nvs --short-build-path
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "calculator_nvs.h"
#include "nvs_fake.h"

/* Never dereferenced: the fakes ignore it */
static struct nvs_fs fake_fs;

/* Clean fakes, module not initialised */
static void nvs_uninit_before(void *fixture) {
	ARG_UNUSED(fixture);

	nvs_fake_reset();
	nvs_fake_init();
	calculator_nvs_deinit();
}

/* Clean fakes, module initialised */
static void nvs_ready_before(void *fixture) {
	ARG_UNUSED(fixture);

	nvs_fake_reset();
	nvs_fake_init();

	/* zassume_ok: skip the test instead of failing it if init breaks */
	zassume_ok(calculator_nvs_init(&fake_fs), "precondition: init must succeed");
}

/*
 * ============================================================================
 * Test Suite: nvs_init
 *
 * Tests for the calculator_nvs_init() function.
 * ============================================================================
 */
ZTEST_SUITE(nvs_init, NULL, NULL, nvs_uninit_before, NULL, NULL);

ZTEST(nvs_init, test_success) {
	int ret = calculator_nvs_init(&fake_fs);

	zassert_ok(ret, "init should succeed, got %d", ret);

	/* call_count: how many times the fake was called */
	zassert_equal(nvs_mount_fake.call_count, 1,
		      "nvs_mount should be called exactly once, got %u",
		      nvs_mount_fake.call_count);
	zassert_equal_ptr(nvs_mount_fake.arg0_val, &fake_fs,
			  "the fs pointer must be forwarded unchanged");
}

ZTEST(nvs_init, test_null_fs_pointer) {
	int ret = calculator_nvs_init(NULL);

	zassert_equal(ret, -EINVAL, "NULL pointer must return -EINVAL, got %d", ret);

	/* The guard must return before the driver is called */
	zassert_equal(nvs_mount_fake.call_count, 0,
		      "nvs_mount must not be called when fs is NULL, got %u calls",
		      nvs_mount_fake.call_count);
}

ZTEST(nvs_init, test_mount_failure) {
	/* return_val: every call returns this value */
	nvs_mount_fake.return_val = -EIO;

	int ret = calculator_nvs_init(&fake_fs);

	zassert_equal(ret, -EIO, "mount error must be returned unchanged, got %d", ret);
}

/*
 * ============================================================================
 * Test Suite: nvs_add
 *
 * Tests for the calculator_nvs_add() function.
 * ============================================================================
 */
ZTEST_SUITE(nvs_add, NULL, NULL, nvs_ready_before, NULL, NULL);

ZTEST(nvs_add, test_add_positive) {
	int result = 0;

	zassert_ok(calculator_nvs_add(2, 3, &result), "add(2, 3) should succeed");
	zassert_equal(result, 5, "2 + 3 should equal 5, got %d", result);
	zassert_equal(nvs_write_fake.call_count, 1,
		      "one add must produce one nvs_write, got %u",
		      nvs_write_fake.call_count);
}

ZTEST(nvs_add, test_add_negative) {
	int result = 0;

	zassert_ok(calculator_nvs_add(-2, -3, &result), "add(-2, -3) should succeed");
	zassert_equal(result, -5, "-2 + -3 should equal -5, got %d", result);
}

ZTEST(nvs_add, test_add_zero) {
	int result = 999;

	zassert_ok(calculator_nvs_add(0, 0, &result), "add(0, 0) should succeed");
	zassert_equal(result, 0, "0 + 0 should equal 0, got %d", result);
}

ZTEST(nvs_add, test_add_null_result) {
	int ret = calculator_nvs_add(1, 2, NULL);

	zassert_equal(ret, -EINVAL, "NULL result must return -EINVAL, got %d", ret);
	zassert_equal(nvs_write_fake.call_count, 0,
		      "nothing may be written for invalid arguments, got %u calls",
		      nvs_write_fake.call_count);
}

/*
 * ============================================================================
 * Test Suite: nvs_sub
 *
 * Tests for the calculator_nvs_sub() function.
 * ============================================================================
 */
ZTEST_SUITE(nvs_sub, NULL, NULL, nvs_ready_before, NULL, NULL);

ZTEST(nvs_sub, test_sub_positive_result) {
	int result = 0;

	zassert_ok(calculator_nvs_sub(10, 4, &result), "sub(10, 4) should succeed");
	zassert_equal(result, 6, "10 - 4 should equal 6, got %d", result);
}

ZTEST(nvs_sub, test_sub_negative_result) {
	int result = 0;

	zassert_ok(calculator_nvs_sub(4, 10, &result), "sub(4, 10) should succeed");
	zassert_equal(result, -6, "4 - 10 should equal -6, got %d", result);
	zassert_ok(calculator_nvs_get_last_result(&result), "read-back should succeed");
	zassert_equal(result, -6, "a negative result must be persisted too, got %d", result);
}

ZTEST(nvs_sub, test_sub_null_result) {
	int ret = calculator_nvs_sub(1, 2, NULL);

	zassert_equal(ret, -EINVAL, "NULL result must return -EINVAL, got %d", ret);
	zassert_equal(nvs_write_fake.call_count, 0,
		      "nothing may be written for invalid arguments, got %u calls",
		      nvs_write_fake.call_count);
}

/*
 * ============================================================================
 * Test Suite: nvs_get
 *
 * Tests for the calculator_nvs_get_last_result() function.
 * ============================================================================
 */
ZTEST_SUITE(nvs_get, NULL, NULL, nvs_ready_before, NULL, NULL);

ZTEST(nvs_get, test_get_after_save) {
	int result = 0;
	int cached = 0;

	zassert_ok(calculator_nvs_add(40, 2, &result), "add must succeed");

	zassert_ok(calculator_nvs_get_last_result(&cached), "read-back should succeed");
	zassert_equal(cached, 42, "round-trip value must survive, got %d", cached);
}

ZTEST(nvs_get, test_get_returns_last_saved) {
	int result = 0;
	int cached = 0;

	zassert_ok(calculator_nvs_add(1, 1, &result), "first add must succeed");
	zassert_ok(calculator_nvs_sub(100, 1, &result), "sub must succeed");

	zassert_ok(calculator_nvs_get_last_result(&cached), "read-back should succeed");
	zassert_equal(cached, 99, "the most recent write must win, got %d", cached);
}

ZTEST(nvs_get, test_get_no_data) {
	int cached = 0;
	int ret = calculator_nvs_get_last_result(&cached);

	zassert_equal(ret, -ENOENT, "an empty store must report -ENOENT, got %d", ret);
}

ZTEST(nvs_get, test_get_null_pointer) {
	int ret = calculator_nvs_get_last_result(NULL);

	zassert_equal(ret, -EINVAL, "NULL result must return -EINVAL, got %d", ret);
	zassert_equal(nvs_read_fake.call_count, 0,
		      "nvs_read must not be called with a NULL destination, got %u calls",
		      nvs_read_fake.call_count);
}
