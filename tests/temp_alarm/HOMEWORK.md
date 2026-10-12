# Temperature Alarm Unit Test Homework — Lecture 08

## Overview

Write unit tests for the `temp_alarm` module — it reads a temperature sensor
and raises an alarm when a reading goes above a threshold.

The exact test cases to implement are specified in `TEST_SPEC.md`. The
skeleton already provides one complete test (`test_status_after_init`) as a
worked example, and declares the remaining 12 stubs with `ztest_test_skip()`
so the binary builds cleanly from the start. Before you can write any test
that reads the sensor, you finish the sensor fake yourself (Task 0).

**Reference**: lecture 08 slides, especially the "When FFF Cannot Reach"
section, and `tests/calculator_nvs/`, which uses the same FFF techniques with
link-time fakes.

## Module Under Test

```text
app/modules/temp_alarm/
├── CMakeLists.txt
├── include/temp_alarm.h     # public API
└── src/temp_alarm.c         # implementation
```

The `temp_alarm` module provides the following API:
```c
int  temp_alarm_init(const struct device *sensor_dev, int threshold);
int  temp_alarm_set_threshold(int threshold);
int  temp_alarm_check(void);
void temp_alarm_reset(void);
int  temp_alarm_get_status(struct temp_alarm_status *status);
```

> Before starting homework, read the full API reference in `include/temp_alarm.h`:
> every return code is documented there. The tests are black-box, but
> `src/temp_alarm.c` is about 120 lines if you want to see why each case behaves
> the way it does.

## Why the Fake Goes Through a `struct device`

`temp_alarm_check()` calls `sensor_sample_fetch_chan()` and
`sensor_channel_get()`. Both are `__syscall` wrappers, inlined into
`temp_alarm.c`, so there is no symbol for FFF to replace:

```text
sensor_sample_fetch_chan(dev, chan)        __syscall -> static inline
  └─ z_impl_sensor_sample_fetch_chan(...)  static inline, inlined into temp_alarm.c
       └─ api->sample_fetch(dev, chan)     a function pointer  <-- can be faked
```

The last step reads a function pointer out of the device you passed to
`temp_alarm_init()`. `src/sensor_fake.c` builds a device whose API table
points at FFF fakes, so every sensor call ends up in a fake. The names of the
fakes do not matter, as long as the API table points at them.

## Provided Infrastructure

`include/sensor_fake.h` and `src/sensor_fake.c` already contain:

- `fake_sensor_channel_get_fake` — the FFF fake for `sensor_driver_api.channel_get`
- `fake_sensor_dev` — the fake `struct device` to pass to `temp_alarm_init()`
- `sensor_fake_set_temperature(temp)` — sets the whole-degree temperature that
  `channel_get` returns
- `sensor_fake_init()` — installs the `custom_fake` on `channel_get` that
  returns that temperature
- `sensor_fake_reset()` — resets every fake, the call history and the
  simulated temperature
- Four `// TODO(l8-task0)` comments where the `sample_fetch` fake is missing

`src/test_basic.c` and `src/test_error_paths.c` already contain:

- All required `#include`s (`zephyr/ztest.h`, `zephyr/fff.h`, `errno.h`,
  `sensor_fake.h`, `temp_alarm.h`)
- A shared `before` hook that resets the fake, calls `sensor_fake_init()` and
  then `temp_alarm_init(&fake_sensor_dev, 30)` before every test
- Six `ZTEST_SUITE` registrations: four in `test_basic.c`, two in
  `test_error_paths.c`
- One complete test (`test_status_after_init`) as a worked example
- Twelve `ZTEST` stubs, each with a `// TODO(l8-task1)` or `// TODO(l8-task2)`
  comment pointing at the matching entry in `TEST_SPEC.md`

Each stub currently calls `ztest_test_skip()`. Replace that line with the
actual test body.

## Running the Tests

Run from the directory that contains `app/` and `tests/`:

```bash
# Build and run (shuffled and ordered scenarios)
west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path

# Verbose per-test output
west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path -v

# Build only (fastest way to find compile errors)
west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path -b
```

---

## Task 0 — Finish the Fake  `git tag l8-task0`

The fake device has no `sample_fetch` yet. Fill in the four `TODO(l8-task0)`
spots:

1. `include/sensor_fake.h` — declare `fake_sensor_sample_fetch` with
   `DECLARE_FAKE_VALUE_FUNC`. It returns `int` and takes
   `(const struct device *, enum sensor_channel)`.
2. `src/sensor_fake.c` — define it with `DEFINE_FAKE_VALUE_FUNC`.
3. `src/sensor_fake.c` — replace `.sample_fetch = NULL` with
   `fake_sensor_sample_fetch` in `fake_sensor_api`.
4. `src/sensor_fake.c` — add `RESET_FAKE(fake_sensor_sample_fetch);` in
   `sensor_fake_reset()`.

**Why this comes first:** until `sample_fetch` is set, `temp_alarm_check()`
calls a NULL function pointer. If you write a test that calls it before Task 0
is done, the whole test binary crashes:

```
ERROR   - native_sim/native         homework.unit.temp_alarm.ordered                   FAILED: rc=-11
```

`rc=-11` is a segmentation fault. In `handler.log` the output ends just after
`START - <your test>`, with no PASS or FAIL for it.

**Acceptance:** the skeleton still builds, and `test_status_after_init` passes
on `qemu_x86`.

```bash
west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path
```

### Tag `l8-task0`

After the fake is finished, tag the commit `l8-task0`.

---

## Task 1 — Happy Paths  `git tag l8-task1`

Open `src/test_basic.c` and fill in each stub `ZTEST` body according to the
**Task 1** section of `TEST_SPEC.md`. Study `test_status_after_init` first — it
shows the pattern. Five tests to write across three suites:

- `temp_alarm_check`: 3 tests (below, at, and above the threshold)
- `temp_alarm_threshold`: 1 test (a new threshold clears the alarm)
- `temp_alarm_reset`: 1 test (reset clears the alarm but keeps the last reading)

For each test:
1. Read the matching row in `TEST_SPEC.md`.
2. Remove `ztest_test_skip()` from the stub.
3. Write the test body. Drive the sensor through the helpers in
   `sensor_fake.h`, and use the fakes to check how the module called it.
4. Re-run Twister and confirm the test passes.

**Acceptance:** all 6 tests in `src/test_basic.c` pass on `qemu_x86`.

```bash
west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path
```

### Tag `l8-task1`

After every Task 1 test passes, tag the commit `l8-task1`.

---

## Task 2 — Error Paths  `git tag l8-task2`

Open `src/test_error_paths.c` and fill in each stub `ZTEST` body according to
the **Task 2** section of `TEST_SPEC.md`. Seven tests to write across two
suites:

- `temp_alarm_sensor_errors`: 3 tests (a fetch failure, a `channel_get`
  failure, a failure that recovers)
- `temp_alarm_invalid_input`: 4 tests (a NULL status, a missing device,
  out-of-range thresholds for `temp_alarm_init()` and
  `temp_alarm_set_threshold()`)

For each test:
1. Read the matching row in `TEST_SPEC.md`.
2. Remove `ztest_test_skip()` from the stub.
3. Write the test body. Make the fakes fail the way the spec describes. There
   is more than one way to do it with FFF; pick the one that reads best.
4. Re-run Twister and confirm the test passes.

**Acceptance:** all 13 tests pass on `qemu_x86`.

```bash
west twister -T tests/temp_alarm -p qemu_x86 -O app/build/twister-out/temp_alarm --short-build-path
```

### Tag `l8-task2`

After every Task 2 test passes, tag the commit `l8-task2`.

---

## Task 3 — Coverage Analysis  `git tag l8-task3`

```bash
# native_sim is built with the host GCC, so --gcov-tool gcov selects the matching host gcov
west twister -T tests/temp_alarm -p native_sim \
    -O app/build/twister-out/temp_alarm \
    --coverage --coverage-tool gcovr --gcov-tool gcov \
    --coverage-basedir app/modules/temp_alarm
```

1. Run the command above at the `l8-task1` commit (`git checkout l8-task1`).
2. Open `app/build/twister-out/temp_alarm/coverage/index.html` in a browser and click into
   `temp_alarm.c`. With only Task 1 done, the expected numbers are:
   ```
   Lines:     79.6%  (43/54)
   Functions: 100.0%  (5/5)
   Branches:  55.6%  (10/18)
   ```
3. Go back to your branch (`git checkout -`), which is at `l8-task2`, and run
   the command again. The expected numbers are:
   ```
   Lines:     100.0%  (54/54)
   Functions: 100.0%  (5/5)
   Branches:  100.0%  (18/18)
   ```
   If your numbers are lower, a test body is likely still calling
   `ztest_test_skip()`, or a test does not match the spec. Go back and check
   each test body against `TEST_SPEC.md` to find the missing one(s).
4. Compare the two reports. Which lines did Task 2 reach that Task 1 could
   not? Notice that function coverage is 100% both times: only the line and
   branch numbers show what was missing.

### Tag `l8-task3`

Include the `app/build/twister-out/temp_alarm/coverage/` directory from the second run in the
commit and tag it `l8-task3`.
