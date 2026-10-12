/*
 * Calculator NVS Persistence Layer - Public API
 *
 * A calculator that writes every result to Non-Volatile Storage (NVS), so the
 * last result survives a power cycle. The arithmetic is trivial; the error
 * handling around the NVS calls is what makes the module worth unit testing.
 */

#ifndef CALCULATOR_NVS_H
#define CALCULATOR_NVS_H

#include <zephyr/kvss/nvs.h>

/** NVS entry ID the last result is stored under. */
#define CALCULATOR_NVS_ID 1

/**
 * @brief Initialise the persistence layer.
 *
 * Mounts the given NVS file system and remembers it for later operations.
 * The struct is treated as opaque: this module never inspects its contents.
 *
 * @param fs Pointer to an already-configured NVS file system.
 *
 * @return 0 on success,
 *         -EINVAL if @p fs is NULL (nvs_mount() is not called),
 *         any negative errno from nvs_mount(), propagated verbatim.
 */
int calculator_nvs_init(struct nvs_fs *fs);

/**
 * @brief Release the NVS handle and reset internal state.
 *
 * After this call the module behaves as if calculator_nvs_init() had never
 * been called. Unit tests use it to reach the uninitialised error paths in
 * isolation, regardless of which test ran before them.
 */
void calculator_nvs_deinit(void);

/**
 * @brief Add two integers and persist the result.
 *
 * @param a First operand.
 * @param b Second operand.
 * @param result Pointer where the sum is stored.
 *
 * @return 0 on success,
 *         -EINVAL if @p result is NULL (nothing is written),
 *         -EACCES if the module was not initialised,
 *         -EIO if NVS accepted fewer bytes than requested,
 *         any negative errno from nvs_write(), propagated verbatim.
 */
int calculator_nvs_add(int a, int b, int *result);

/**
 * @brief Subtract two integers and persist the result.
 *
 * @param a Minuend.
 * @param b Subtrahend.
 * @param result Pointer where the difference is stored.
 *
 * @return Same set of codes as calculator_nvs_add().
 */
int calculator_nvs_sub(int a, int b, int *result);

/**
 * @brief Read the last persisted result back from NVS.
 *
 * @param result Pointer where the stored value is written.
 *
 * @return 0 on success,
 *         -EINVAL if @p result is NULL (nvs_read() is not called),
 *         -EACCES if the module was not initialised,
 *         -EIO if NVS returned fewer bytes than a stored result,
 *         any negative errno from nvs_read(), propagated verbatim
 *         (-ENOENT when nothing has been stored yet).
 */
int calculator_nvs_get_last_result(int *result);

#endif /* CALCULATOR_NVS_H */
