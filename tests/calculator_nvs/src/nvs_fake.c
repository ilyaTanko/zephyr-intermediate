/*
 * NVS Fake - definitions and simulated storage
 */

#include "nvs_fake.h"

#include <errno.h>
#include <stdbool.h>
#include <zephyr/fff.h>
#include <zephyr/toolchain.h>

/* Exactly one of these per test binary. */
DEFINE_FFF_GLOBALS;

/* Creates the nvs_*_fake structs */
DEFINE_FAKE_VALUE_FUNC(int, nvs_mount, struct nvs_fs *);
DEFINE_FAKE_VALUE_FUNC(ssize_t, nvs_read, struct nvs_fs *, uint16_t, void *, size_t);
DEFINE_FAKE_VALUE_FUNC(ssize_t, nvs_write, struct nvs_fs *, uint16_t, const void *, size_t);

/* ---- Simulated NVS state ------------------------------------------------ */

/* One int is enough: calculator_nvs only stores CALCULATOR_NVS_ID */
static int simulated_nvs_storage;
static bool has_data;

/* ---- custom_fake implementations ---------------------------------------- */

/* Copies the value while the caller's pointer is still valid */
static ssize_t custom_nvs_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len) {
	ARG_UNUSED(fs);
	ARG_UNUSED(id);

	if (len != sizeof(int)) {
		return -EINVAL;
	}

	simulated_nvs_storage = *(const int *)data;
	has_data = true;

	return (ssize_t)len;
}

/* Returns -ENOENT until something is written, like the real NVS */
static ssize_t custom_nvs_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len) {
	ARG_UNUSED(fs);
	ARG_UNUSED(id);

	if (!has_data) {
		return -ENOENT;
	}

	if (len != sizeof(int)) {
		return -EINVAL;
	}

	*(int*)data = simulated_nvs_storage;

	return (ssize_t)len;
}

/* ---- Public helpers ------------------------------------------------------ */

void nvs_fake_init(void) {
	nvs_read_fake.custom_fake = custom_nvs_read;
	nvs_write_fake.custom_fake = custom_nvs_write;
}

void nvs_fake_reset(void) {
	RESET_FAKE(nvs_mount);
	RESET_FAKE(nvs_read);
	RESET_FAKE(nvs_write);

	/* Clears fff.call_history, shared by all fakes */
	FFF_RESET_HISTORY();

	simulated_nvs_storage = 0;
	has_data = false;
}

