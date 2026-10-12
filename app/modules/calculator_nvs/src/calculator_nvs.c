/*
 * Calculator NVS Persistence Layer - Implementation
 *
 * Every public entry point guards its inputs, dispatches to the NVS API, and
 * propagates whatever the storage layer reports. Those guards and propagation
 * paths are the interesting part for unit tests: on a healthy filesystem most
 * of them never execute.
 */

#include "calculator_nvs.h"

#include <errno.h>
#include <stddef.h>
#include <zephyr/kvss/nvs.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(calculator_nvs);

/* Module-static state: the mounted file system, or NULL before init. */
static struct nvs_fs *ctx_fs;

int calculator_nvs_init(struct nvs_fs *fs)
{
	int rc;

	if (fs == NULL) {
		LOG_ERR("NVS file system pointer is NULL");
		return -EINVAL;
	}

	rc = nvs_mount(fs);
	if (rc < 0) {
		LOG_ERR("Failed to mount NVS: %d", rc);
		return rc;
	}

	ctx_fs = fs;
	LOG_INF("Calculator NVS persistence initialised");
	return 0;
}

void calculator_nvs_deinit(void)
{
	ctx_fs = NULL;
	LOG_INF("Calculator NVS persistence released");
}

/* Shared write path for add() and sub(). */
static int calculator_nvs_save(int result)
{
	ssize_t rc;

	if (ctx_fs == NULL) {
		LOG_ERR("NVS not initialised, cannot save result");
		return -EACCES;
	}

	rc = nvs_write(ctx_fs, CALCULATOR_NVS_ID, &result, sizeof(result));
	if (rc < 0) {
		LOG_ERR("Failed to save result to NVS: %d", (int)rc);
		return (int)rc;
	}

	if (rc != (ssize_t)sizeof(result)) {
		LOG_ERR("Short write to NVS: %d of %u bytes", (int)rc, (unsigned int)sizeof(result));
		return -EIO;
	}

	LOG_INF("Result %d saved to NVS", result);
	return 0;
}

int calculator_nvs_add(int a, int b, int *result)
{
	if (result == NULL) {
		LOG_ERR("result pointer is NULL");
		return -EINVAL;
	}

	*result = a + b;
	return calculator_nvs_save(*result);
}

int calculator_nvs_sub(int a, int b, int *result)
{
	if (result == NULL) {
		LOG_ERR("result pointer is NULL");
		return -EINVAL;
	}

	*result = a - b;
	return calculator_nvs_save(*result);
}

int calculator_nvs_get_last_result(int *result)
{
	ssize_t rc;

	if (result == NULL) {
		LOG_ERR("result pointer is NULL");
		return -EINVAL;
	}

	if (ctx_fs == NULL) {
		LOG_ERR("NVS not initialised");
		return -EACCES;
	}

	rc = nvs_read(ctx_fs, CALCULATOR_NVS_ID, result, sizeof(*result));
	if (rc < 0) {
		LOG_ERR("Failed to read last result from NVS: %d", (int)rc);
		return (int)rc;
	}

	if (rc != (ssize_t)sizeof(*result)) {
		LOG_ERR("Short read from NVS: %d of %u bytes", (int)rc,
			(unsigned int)sizeof(*result));
		return -EIO;
	}

	LOG_INF("Last result read from NVS: %d", *result);
	return 0;
}
