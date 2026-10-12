/*
 * Calculator NVS Unit Tests - Error Paths and Advanced FFF
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

	zassume_ok(calculator_nvs_init(&fake_fs), "precondition: init must succeed");
}

/*
 * ============================================================================
 * Test Suite: nvs_uninitialized
 *
 * Calls made before calculator_nvs_init().
 * ============================================================================
 */
ZTEST_SUITE(nvs_uninitialized, NULL, NULL, nvs_uninit_before, NULL, NULL);

ZTEST(nvs_uninitialized, test_add_without_init) {
	int result = 0;
	int ret = calculator_nvs_add(20, 22, &result);

	zassert_equal(ret, -EACCES, "add before init must return -EACCES, got %d", ret);
	zassert_equal(nvs_write_fake.call_count, 0,
		      "nvs_write must not be called before init, got %u calls",
		      nvs_write_fake.call_count);
}

ZTEST(nvs_uninitialized, test_get_without_init) {
	int result = 0;
	int ret = calculator_nvs_get_last_result(&result);

	zassert_equal(ret, -EACCES, "get before init must return -EACCES, got %d", ret);
	zassert_equal(nvs_read_fake.call_count, 0,
		      "nvs_read must not be called before init, got %u calls",
		      nvs_read_fake.call_count);
}

/*
 * ============================================================================
 * Test Suite: nvs_error_read_write
 *
 * Truncated transfers, injected through the fakes.
 * ============================================================================
 */
ZTEST_SUITE(nvs_error_read_write, NULL, NULL, nvs_ready_before, NULL, NULL);

/* custom_fakes that report a 1-byte transfer instead of sizeof(int) */
static ssize_t truncating_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len) {
	ARG_UNUSED(fs);
	ARG_UNUSED(id);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	return 1;
}

static ssize_t truncating_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len) {
	ARG_UNUSED(fs);
	ARG_UNUSED(id);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	return 1;
}

ZTEST(nvs_error_read_write, test_short_write_reports_eio) {
	int result = 0;

	/* Replaces the custom_fake installed by nvs_fake_init() */
	nvs_write_fake.custom_fake = truncating_write;

	int ret = calculator_nvs_add(2, 2, &result);

	zassert_equal(ret, -EIO, "a truncated write must be reported as -EIO, got %d", ret);
}

ZTEST(nvs_error_read_write, test_short_read_reports_eio) {
	int result = 0;

	nvs_read_fake.custom_fake = truncating_read;

	int ret = calculator_nvs_get_last_result(&result);

	zassert_equal(ret, -EIO, "a truncated read must be reported as -EIO, got %d", ret);
}

/*
 * ============================================================================
 * Test Suite: nvs_fff_advanced
 *
 * Argument capture, call history and return sequences.
 * ============================================================================
 */
ZTEST_SUITE(nvs_fff_advanced, NULL, NULL, nvs_ready_before, NULL, NULL);

/* arg<N>_val is zero-indexed: nvs_write(fs = arg0, id = arg1, data = arg2, len = arg3) */
ZTEST(nvs_fff_advanced, test_verify_nvs_id_used) {
	int result = 0;

	zassert_ok(calculator_nvs_add(21, 21, &result), "add must succeed");

	zassert_equal(nvs_write_fake.arg1_val, CALCULATOR_NVS_ID,
		      "the module must write to entry id %u, got %u", CALCULATOR_NVS_ID,
		      nvs_write_fake.arg1_val);
	zassert_equal(nvs_write_fake.arg3_val, sizeof(int),
		      "the module must offer sizeof(int) bytes, got %u",
		      (unsigned int)nvs_write_fake.arg3_val);
}

/* fff.call_history is shared by every fake, so it shows the order across fakes */
ZTEST(nvs_fff_advanced, test_call_history_records_order)
{
	int result = 0;

	/* Entry 0 is the nvs_mount() call made by the before hook */
	zassert_equal(fff.call_history[0], (fff_function_t)nvs_mount,
		      "the first recorded call should be nvs_mount");

	zassert_ok(calculator_nvs_add(90, 9, &result), "add must succeed");
	zassert_ok(calculator_nvs_get_last_result(&result), "get must succeed");

	zassert_equal(fff.call_history[1], (fff_function_t)nvs_write,
		      "add() must write before anything reads");
	zassert_equal(fff.call_history[2], (fff_function_t)nvs_read,
		      "get_last_result() must be the third recorded call");
	zassert_is_null(fff.call_history[3], "no fourth call was expected");
	zassert_equal(result, 99, "the round-trip value must be preserved, got %d", result);
}

/* arg<N>_history keeps the argument of every call, not only the last one */
ZTEST(nvs_fff_advanced, test_arg_history_records_every_call) {
	int result = 0;

	zassert_ok(calculator_nvs_add(1, 1, &result), "add must succeed");
	zassert_ok(calculator_nvs_sub(9, 4, &result), "sub must succeed");

	zassert_equal(nvs_write_fake.call_count, 2, "two operations must produce two writes, got %u", nvs_write_fake.call_count);
	zassert_equal(nvs_write_fake.arg1_history[0], CALCULATOR_NVS_ID, "the first write must use entry id %u", CALCULATOR_NVS_ID);
	zassert_equal(nvs_write_fake.arg1_history[1], CALCULATOR_NVS_ID, "every write must use the same entry id");
	zassert_equal(nvs_write_fake.arg_histories_dropped, 0, "no argument history should have been dropped, got %u",
		nvs_write_fake.arg_histories_dropped);
}

/* SET_RETURN_SEQ: one return value per call.
 * An exhausted sequence repeats its last element instead of falling back to return_val.
 */
ZTEST(nvs_fff_advanced, test_return_val_sequence) {
	static ssize_t seq[] = {-EIO, -ENOSPC};
	int result = 0;

	/* Clear custom_fake, or the sequence is ignored */
	nvs_write_fake.custom_fake = NULL;
	SET_RETURN_SEQ(nvs_write, seq, ARRAY_SIZE(seq));

	zassert_equal(calculator_nvs_add(1, 0, &result), -EIO, "call 1: seq[0] is -EIO");
	zassert_equal(calculator_nvs_add(2, 0, &result), -ENOSPC, "call 2: seq[1] is -ENOSPC");
	zassert_equal(calculator_nvs_add(3, 0, &result), -ENOSPC, "call 3: an exhausted sequence repeats its last element");

	zassert_equal(nvs_write_fake.call_count, 3, "three adds must produce three writes, got %u", nvs_write_fake.call_count);
}

/* SET_CUSTOM_FAKE_SEQ: one function per call. It outranks custom_fake, so the
 * callback from nvs_fake_init() is not used.
 */
static int seq_stored;

static ssize_t seq_write_ok(struct nvs_fs *fs, uint16_t id, const void *data, size_t len) {
	ARG_UNUSED(fs);
	ARG_UNUSED(id);

	seq_stored = *(const int *)data;

	return (ssize_t)len;
}

static ssize_t seq_write_short(struct nvs_fs *fs, uint16_t id, const void *data, size_t len) {
	ARG_UNUSED(fs);
	ARG_UNUSED(id);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	return 1;
}

ZTEST(nvs_fff_advanced, test_custom_fake_seq_runs_each_function) {
	static ssize_t (*write_seq[])(struct nvs_fs *, uint16_t, const void *, size_t) = {
		seq_write_ok,
		seq_write_short,
	};
	int result = 0;

	seq_stored = 0;
	SET_CUSTOM_FAKE_SEQ(nvs_write, write_seq, ARRAY_SIZE(write_seq));

	zassert_ok(calculator_nvs_add(2, 3, &result), "call 1: seq_write_ok stores 5");
	zassert_equal(calculator_nvs_add(1, 1, &result), -EIO, "call 2: a short write must be reported as -EIO");
	zassert_equal(calculator_nvs_add(4, 4, &result), -EIO, "call 3: an exhausted sequence repeats its last function");
	zassert_equal(seq_stored, 5, "call 1 must have written 5, got %d", seq_stored);
}
