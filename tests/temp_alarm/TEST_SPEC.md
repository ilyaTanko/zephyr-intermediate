# Temperature Alarm Unit Test Specification

The `temp_alarm` module keeps its state (threshold, last reading, alarm count,
alarm flag, last error) in a `static` struct inside `temp_alarm.c`. Tests can
only inspect it through `temp_alarm_get_status()`, and can only change it
through the public API and the sensor fake.

Shorthands used below:
- `status` — a `struct temp_alarm_status status` declared in the test
- `get_status()` — `temp_alarm_get_status(&status)`, called before checking any `status` field
- `check()` — `temp_alarm_check()`
- `init(dev, N)` — `temp_alarm_init(dev, N)`
- `set_threshold(N)` — `temp_alarm_set_threshold(N)`
- `set_temp(N)` — `sensor_fake_set_temperature(N)`
- `&fsd` — `&fake_sensor_dev`

**Before hook** (every suite, already in both test files):
`sensor_fake_reset()` → `sensor_fake_init()` → `zassume_ok(temp_alarm_init(&fsd, 30))`

---

## Provided — runs before Task 0

### Suite `temp_alarm_get_status`

| #   | Scenario                       | Precondition       | Input          | Expected |
| --- | ------------------------------ | ------------------ | -------------- | -------- |
| 1   | Status after init *(provided)* | None (before hook) | `get_status()` | `0`      |

**Verify #1 (provided):** `status.last_temp == 0`, `status.alarm_count == 0`,
`status.is_alarming == false` and `status.last_error == 0`.

---

## Task 1 — Happy Paths (`src/test_basic.c`)

### Suite `temp_alarm_check`

| #   | Scenario            | Precondition   | Input     | Expected |
| --- | ------------------- | -------------- | --------- | -------- |
| 1   | Below the threshold | `set_temp(20)` | `check()` | `0`      |
| 2   | At the threshold    | `set_temp(30)` | `check()` | `0`      |
| 3   | Above the threshold | `set_temp(40)` | `check()` | `0`      |

**Verify #1:** `status.last_temp == 20` and `status.is_alarming == false`. The
module fetched a sample exactly once, and read the ambient temperature
channel (`SENSOR_CHAN_AMBIENT_TEMP`).

**Verify #2:** `status.is_alarming == false` and `status.alarm_count == 0`. The
alarm needs a reading strictly above the threshold.

**Verify #3:** `status.last_temp == 40`, `status.is_alarming == true` and
`status.alarm_count == 1`.

### Suite `temp_alarm_threshold`

| #   | Scenario                       | Precondition              | Input                               | Expected |
| --- | ------------------------------ | ------------------------- | ----------------------------------- | -------- |
| 1   | New threshold clears the alarm | `set_temp(40)`, `check()` | `set_threshold(50)`, then `check()` | `0`, `0` |

**Verify #1:** the first `check()` raises the alarm. Right after
`set_threshold(50)`, `status.is_alarming == false`. After the second
`check()`, `status.is_alarming == false` (40 C is below 50 C) and
`status.alarm_count == 1` (still only the first alarm).

### Suite `temp_alarm_reset`

| #   | Scenario               | Precondition                  | Input                | Expected |
| --- | ---------------------- | ----------------------------- | -------------------- | -------- |
| 1   | Reset clears the alarm | `set_temp(40)`, `check()` × 3 | `temp_alarm_reset()` | —        |

**Verify #1:** the three checks leave `status.alarm_count == 3`.
After the reset, `status.is_alarming == false`, `status.alarm_count == 0`,
`status.last_error == 0`, and `status.last_temp == 40` (reset keeps the last
reading).

---

## Task 2 — Error Paths (`src/test_error_paths.c`)

### Suite `temp_alarm_sensor_errors`

| #   | Scenario              | Precondition                                            | Input           | Expected         |
| --- | --------------------- | ------------------------------------------------------- | --------------- | ---------------- |
| 1   | Fetch failure         | `sample_fetch` returns `-EIO`                           | `check()`       | `-EIO`           |
| 2   | `channel_get` failure | `channel_get` returns `-EIO`                            | `check()`       | `-EIO`           |
| 3   | Fail, then recover    | `sample_fetch` returns `-EIO`, then `0`; `set_temp(20)` | `check()` twice | `-EIO`, then `0` |

**Verify #1:** `status.last_error == -EIO`. The module stops after the failed
fetch: `channel_get` is never called, so no stale value is read.

**Verify #2:** `status.last_error == -EIO`, and the sample was still fetched
exactly once. Note that `sensor_fake_init()` has already configured
`channel_get` in the before hook.

**Verify #3:** the first `check()` returns `-EIO` and the second returns `0`.
After the second check, `status.last_temp == 20`, `status.is_alarming == false`
and `status.last_error == -EIO` (the last error is kept until a reset).

### Suite `temp_alarm_invalid_input`

| #   | Scenario                    | Precondition       | Input                                          | Expected             |
| --- | --------------------------- | ------------------ | ---------------------------------------------- | -------------------- |
| 1   | NULL status                 | None (before hook) | `temp_alarm_get_status(NULL)`                  | `-EINVAL`            |
| 2   | No device                   | None (before hook) | `init(NULL, 30)`                               | `-ENODEV`            |
| 3   | Init threshold out of range | None (before hook) | `init(&fsd, -1)`, then `init(&fsd, 101)`       | `-EINVAL`, `-EINVAL` |
| 4   | New threshold out of range  | None (before hook) | `set_threshold(-1)`, then `set_threshold(101)` | `-EINVAL`, `-EINVAL` |

**Verify #1:** the return value is exactly `-EINVAL`.

**Verify #2:** the return value is exactly `-ENODEV`.

**Verify #3:** both calls return exactly `-EINVAL`.

**Verify #4:** both calls return exactly `-EINVAL`, and the rejected values
did not change the threshold: 30 C is still in force.
