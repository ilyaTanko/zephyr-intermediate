# Unit Tests — `calculator_nvs`

Unit tests for the `calculator_nvs` module, demonstrating FFF fakes on
`qemu_x86` and `native_sim`.

## What is tested

The `calculator_nvs` module is a calculator that saves every result to
Non-Volatile Storage (NVS):

```c
int  calculator_nvs_init(struct nvs_fs *fs);
void calculator_nvs_deinit(void);
int  calculator_nvs_add(int a, int b, int *result);
int  calculator_nvs_sub(int a, int b, int *result);
int  calculator_nvs_get_last_result(int *result);
```

All functions except `calculator_nvs_deinit` return `0` on success or a
negative errno on failure. Every return code is documented in
`app/modules/calculator_nvs/include/calculator_nvs.h`.

## What is faked, and why

The module calls three NVS functions: `nvs_mount()`, `nvs_write()` and
`nvs_read()`. With the real NVS the tests would need a flash device, and the
error handling around those calls could never run: a working flash chip does
not return `-EIO`, or accept 1 byte out of 4, when a test asks it to.

`src/nvs_fake.c` replaces all three functions with FFF fakes. The fakes have
the same names and signatures as the real functions, so the linker uses them
instead. That is why `prj.conf` leaves `CONFIG_NVS` disabled on purpose.

Helpers declared in `include/nvs_fake.h`:

| Helper                          | Purpose                                                                        |
| ------------------------------- | ------------------------------------------------------------------------------ |
| `nvs_fake_reset()`              | Reset all three fakes, the call history and the simulated storage              |
| `nvs_fake_init()`               | Install the `custom_fake` callbacks that act as a one-entry NVS                |

## Code structure

```
tests/calculator_nvs/
├── CMakeLists.txt          # Links calculator_nvs.c with the fakes and both test files
├── Kconfig                 # Sources Kconfig.zephyr for the test build
├── prj.conf                # CONFIG_ZTEST=y, CONFIG_ZTEST_SHUFFLE=y, CONFIG_LOG=y; CONFIG_NVS stays off
├── testcase.yaml           # Two scenarios: shuffled, ordered
├── README.md               # This file
├── include/
│   └── nvs_fake.h          # DECLARE_FAKE_VALUE_FUNC for the three fakes, helper API
└── src/
    ├── nvs_fake.c          # DEFINE_FFF_GLOBALS, DEFINE_FAKE_VALUE_FUNC, custom fakes
    ├── test_basic.c        # Happy paths: 4 suites, 14 tests
    └── test_error_paths.c  # Error paths and advanced FFF: 3 suites, 9 tests
```

## Test suites

### `test_basic.c` — happy paths

| Suite      | What it covers                                                                                                    |
| ---------- | ----------------------------------------------------------------------------------------------------------------- |
| `nvs_init` | Successful mount; NULL pointer guard, with `nvs_mount` never called; mount error passed through unchanged         |
| `nvs_add`  | Positive, negative and zero sums; NULL result guard                                                               |
| `nvs_sub`  | Positive and negative results; a negative result is saved too; NULL result guard                                  |
| `nvs_get`  | Save then read back; the most recent write wins; an empty store returns `-ENOENT`; NULL guard, with no `nvs_read` |

### `test_error_paths.c` — error paths and advanced FFF

| Suite                  | What it covers                                                                                                    |
| ---------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `nvs_uninitialized`    | `-EACCES` from add and get before init, with no call into NVS                                                     |
| `nvs_error_read_write` | A truncated write or read reported as `-EIO`                                                                      |
| `nvs_fff_advanced`     | `arg<N>_val`, `fff.call_history`, `arg<N>_history`, `SET_RETURN_SEQ` and `SET_CUSTOM_FAKE_SEQ`                    |

## Techniques demonstrated

| Technique                                                             | Example                                                                    |
| --------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| `return_val`                                                          | `nvs_init.test_mount_failure`                                              |
| `call_count`                                                          | `nvs_init.test_null_fs_pointer`, `nvs_uninitialized.test_add_without_init` |
| `custom_fake` as a small working NVS                                  | `nvs_fake.c`; round trips in `nvs_get`                                     |
| A `custom_fake` installed by a single test                            | `nvs_error_read_write.test_short_write_reports_eio`                        |
| `arg<N>_val`                                                          | `nvs_fff_advanced.test_verify_nvs_id_used`                                 |
| `fff.call_history`                                                    | `nvs_fff_advanced.test_call_history_records_order`                         |
| `arg<N>_history`                                                      | `nvs_fff_advanced.test_arg_history_records_every_call`                     |
| `SET_RETURN_SEQ`                                                      | `nvs_fff_advanced.test_return_val_sequence`                                |
| `SET_CUSTOM_FAKE_SEQ`                                                 | `nvs_fff_advanced.test_custom_fake_seq_runs_each_function`                 |
| Fakes reset in `before()`: `nvs_fake_reset()`, then `nvs_fake_init()` | Every suite                                                                |
| `zassume_ok` in `before()` for preconditions                          | Every suite that needs a successful init                                   |

## Platforms

- `qemu_x86` — runs through QEMU and is the recommended platform on Windows.
- `native_sim` — compiles Zephyr into a native executable and requires Linux.

`CONFIG_ZTEST_SHUFFLE=y` (default scenario) randomizes suite and test order to
catch hidden ordering dependencies.

## Running the tests

```bash
# All scenarios (shuffled + ordered) on QEMU
west twister -T tests/calculator_nvs -p qemu_x86 -O app/build/twister-out/calculator_nvs --short-build-path

# Verbose per-test output
west twister -T tests/calculator_nvs -p qemu_x86 -O app/build/twister-out/calculator_nvs --short-build-path -v

# Deterministic order — useful for live demos or failure bisection
west twister -T tests/calculator_nvs -p qemu_x86 -O app/build/twister-out/calculator_nvs --short-build-path -s example.unit.calculator_nvs.ordered

# Build only (fastest compile check, no execution)
west twister -T tests/calculator_nvs -p qemu_x86 -O app/build/twister-out/calculator_nvs --short-build-path -b

# Coverage report (Linux/native_sim)
# native_sim is built with the host GCC, so --gcov-tool gcov selects the matching host gcov
west twister -T tests/calculator_nvs -p native_sim -O app/build/twister-out/calculator_nvs --coverage --coverage-tool gcovr --gcov-tool gcov --coverage-basedir app/modules/calculator_nvs
```

## Expected output

### Twister summary

Output of `west twister -T tests/calculator_nvs -p qemu_x86`:

```
INFO    - 2 of 2 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings.
INFO    - 46 of 46 executed test cases passed (100.00%) on 1 out of total 1473 platforms (0.07%).
```

### Coverage

Output of the coverage command above, for `calculator_nvs.c`:

```
Lines:     100.0%  (56/56)
Functions: 100.0%  (6/6)
Branches:  100.0%  (22/22)
```

With only `test_basic.c` (the 14 happy-path tests), the same module scores
82.1% lines (46/56), 77.3% branches (17/22) and still 100% functions. All ten
uncovered lines are error handling, which only the fakes can reach.

### native_sim output

Output of executing tests on `native_sim` by running the compiled executable
directly
`./app/build/twister-out/calculator_nvs/native_sim_native/host_gnu/.../tests/calculator_nvs/example.unit.calculator_nvs.ordered/zephyr/zephyr.exe`
(the `...` part is the path of this project inside your west workspace). The
start of the run, and the summary at the end:

```
*** Booting Zephyr OS build v4.4.0 ***
Running TESTSUITE nvs_add
===================================================================
START - test_add_negative
[00:00:00.000,000] <inf> calculator_nvs: Calculator NVS persistence initialised
[00:00:00.000,000] <inf> calculator_nvs: Result -5 saved to NVS
 PASS - test_add_negative in 0.000 seconds
...

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [nvs_add]: pass = 4, fail = 0, skip = 0, total = 4 duration = 0.000 seconds
 - PASS - [nvs_add.test_add_negative] duration = 0.000 seconds
 - PASS - [nvs_add.test_add_null_result] duration = 0.000 seconds
 - PASS - [nvs_add.test_add_positive] duration = 0.000 seconds
 - PASS - [nvs_add.test_add_zero] duration = 0.000 seconds

SUITE PASS - 100.00% [nvs_error_read_write]: pass = 2, fail = 0, skip = 0, total = 2 duration = 0.000 seconds
 - PASS - [nvs_error_read_write.test_short_read_reports_eio] duration = 0.000 seconds
 - PASS - [nvs_error_read_write.test_short_write_reports_eio] duration = 0.000 seconds

SUITE PASS - 100.00% [nvs_fff_advanced]: pass = 5, fail = 0, skip = 0, total = 5 duration = 0.000 seconds
 - PASS - [nvs_fff_advanced.test_arg_history_records_every_call] duration = 0.000 seconds
 - PASS - [nvs_fff_advanced.test_call_history_records_order] duration = 0.000 seconds
 - PASS - [nvs_fff_advanced.test_custom_fake_seq_runs_each_function] duration = 0.000 seconds
 - PASS - [nvs_fff_advanced.test_return_val_sequence] duration = 0.000 seconds
 - PASS - [nvs_fff_advanced.test_verify_nvs_id_used] duration = 0.000 seconds

SUITE PASS - 100.00% [nvs_get]: pass = 4, fail = 0, skip = 0, total = 4 duration = 0.000 seconds
 - PASS - [nvs_get.test_get_after_save] duration = 0.000 seconds
 - PASS - [nvs_get.test_get_no_data] duration = 0.000 seconds
 - PASS - [nvs_get.test_get_null_pointer] duration = 0.000 seconds
 - PASS - [nvs_get.test_get_returns_last_saved] duration = 0.000 seconds

SUITE PASS - 100.00% [nvs_init]: pass = 3, fail = 0, skip = 0, total = 3 duration = 0.000 seconds
 - PASS - [nvs_init.test_mount_failure] duration = 0.000 seconds
 - PASS - [nvs_init.test_null_fs_pointer] duration = 0.000 seconds
 - PASS - [nvs_init.test_success] duration = 0.000 seconds

SUITE PASS - 100.00% [nvs_sub]: pass = 3, fail = 0, skip = 0, total = 3 duration = 0.000 seconds
 - PASS - [nvs_sub.test_sub_negative_result] duration = 0.000 seconds
 - PASS - [nvs_sub.test_sub_null_result] duration = 0.000 seconds
 - PASS - [nvs_sub.test_sub_positive_result] duration = 0.000 seconds

SUITE PASS - 100.00% [nvs_uninitialized]: pass = 2, fail = 0, skip = 0, total = 2 duration = 0.000 seconds
 - PASS - [nvs_uninitialized.test_add_without_init] duration = 0.000 seconds
 - PASS - [nvs_uninitialized.test_get_without_init] duration = 0.000 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
```
