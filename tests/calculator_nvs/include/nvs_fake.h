/*
 * NVS Fake - FFF declarations and helper API
 *
 * DECLARE_* is here, so both test files share the fakes. DEFINE_* is in
 * nvs_fake.c, exactly once.
 */

#ifndef NVS_FAKE_H
#define NVS_FAKE_H

#include <stdbool.h>
#include <zephyr/fff.h>
#include <zephyr/kvss/nvs.h>

/* Same signatures as the real API: nvs_read() and nvs_write() return ssize_t */
DECLARE_FAKE_VALUE_FUNC(int, nvs_mount, struct nvs_fs *);
DECLARE_FAKE_VALUE_FUNC(ssize_t, nvs_read, struct nvs_fs *, uint16_t, void *, size_t);
DECLARE_FAKE_VALUE_FUNC(ssize_t, nvs_write, struct nvs_fs *, uint16_t, const void *, size_t);

/**
 * @brief Install the custom_fake callbacks that emulate a one-entry NVS.
 *
 * Call this after nvs_fake_reset() in a before() hook: RESET_FAKE() clears
 * custom_fake, so the callbacks have to be re-installed every time.
 */
void nvs_fake_init(void);


/** @brief Reset all three fakes, the call history and the simulated store. */
void nvs_fake_reset(void);

#endif /* NVS_FAKE_H */
