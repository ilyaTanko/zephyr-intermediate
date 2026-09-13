#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sflist.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/task_wdt/task_wdt.h>

#include <math.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 2048
#define SENSOR_COUNT 18
#define SENSOR_PERIOD_MS 100
#define LOGGER_PERIOD_MS 200
#define LOGGER_WDT_PERIOD_MS 1200
#define LOGGER_STALL_AFTER_RECEIVED 5
#define LOGGER_STALL_MS 1000
#define HEALTHCHECK_PERIOD_MS 250
#define HEALTHCHECK_WARNING_THRESHOLD_PERCENT 75

struct vector3 {
    int32_t x;
    int32_t y;
    int32_t z;
};

struct sensor_data {
    struct vector3 acceleration_mms2;
    uint32_t timestamp_ms;
    uint8_t seq;
};

struct motion_state {
    struct vector3 position_mm;
    struct vector3 velocity_mms;
    uint32_t timestamp_ms;
    bool initialized;
};

static uint32_t vector_magnitude(struct vector3 const* vector) {
    int64_t squared = (int64_t)vector->x * vector->x +
                      (int64_t)vector->y * vector->y +
                      (int64_t)vector->z * vector->z;

    return (uint32_t)sqrt((double)squared);
}

static struct vector3 integrate_motion(struct motion_state* state,
                                       struct vector3 const* acceleration,
                                       uint32_t timestamp_ms) {
    struct vector3 displacement_mm = {0, 0, 0};

    if (!state->initialized) {
        state->timestamp_ms = timestamp_ms;
        state->initialized = true;
        return displacement_mm;
    }

    uint32_t elapsed_ms = timestamp_ms - state->timestamp_ms;
    int64_t elapsed_squared = (int64_t)elapsed_ms * elapsed_ms;
    int32_t* displacement_axes = &displacement_mm.x;
    int32_t* position_axes = &state->position_mm.x;
    int32_t* velocity_axes = &state->velocity_mms.x;
    int32_t const* acceleration_axes = &acceleration->x;

    for (int axis = 0; axis < 3; axis++) {
        displacement_axes[axis] = (int32_t)((int64_t)velocity_axes[axis] * elapsed_ms / 1000 +
                                            (int64_t)acceleration_axes[axis] * elapsed_squared /
                                                2000000);
        position_axes[axis] += displacement_axes[axis];
        velocity_axes[axis] +=
            (int32_t)((int64_t)acceleration_axes[axis] * elapsed_ms / 1000);
    }

    state->timestamp_ms = timestamp_ms;
    return displacement_mm;
}

// Listener: runs in publisher context.
static void display_listener_cb(struct zbus_channel const* chan) {
    struct sensor_data const* msg = (struct sensor_data const*)zbus_chan_const_msg(chan);
    LOG_INF("[DISPLAY] seq=%u accel=(%d, %d, %d) mm/s2",
            msg->seq,
            msg->acceleration_mms2.x,
            msg->acceleration_mms2.y,
            msg->acceleration_mms2.z);
}

ZBUS_LISTENER_DEFINE(display_listener, display_listener_cb);

// Message subscriber: receives message copies, not only channel notifications.
ZBUS_MSG_SUBSCRIBER_DEFINE(logger_sub);

// Channel
ZBUS_CHAN_DEFINE(sensor_chan, struct sensor_data,
                 NULL, NULL,
                 ZBUS_OBSERVERS(display_listener, logger_sub),
                 ZBUS_MSG_INIT(.acceleration_mms2 = {0, 0, 0},
                               .timestamp_ms = 0,
                               .seq = 0));

// Publisher sensor thread
static void sensor_thread_fn(void* p1, void* p2, void* p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
        struct sensor_data data = {
            .acceleration_mms2 = {
                .x = 1000 + (i % 3) * 500,
                .y = (i % 2) ? -500 : 500,
                .z = 250,
            },
            .timestamp_ms = k_uptime_get_32(),
            .seq = i,
        };

        LOG_INF("[SENSOR] publish seq=%u accel=(%d, %d, %d) mm/s2",
                data.seq,
                data.acceleration_mms2.x,
                data.acceleration_mms2.y,
                data.acceleration_mms2.z);

        int ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(100));
        if (ret != 0) {
            LOG_WRN("[SENSOR] publish failed ret=%d", ret);
        }

        k_msleep(SENSOR_PERIOD_MS);
    }

    LOG_INF("[SENSOR] done");
}

static void logger_task_wdt_cb(int channel_id, void* user_data) {
    LOG_ERR("[LOGGER-WDT] callback triggered: channel=%d thread=%s",
            channel_id,
            k_thread_name_get((k_tid_t)user_data));
}

static void healthcheck_thread_fn(void* p1, void* p2, void* p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("[QUEUE] healthcheck thread started");

    while (true) {
        uint32_t used_msgs = sys_sflist_len(&logger_sub.message_fifo->_queue.data_q);
        uint32_t fill_percent = (used_msgs * 100U) / CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE;

        LOG_INF("[QUEUE] fill=%u/%u (%u%%)", used_msgs, CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE, fill_percent);

        if (fill_percent >= HEALTHCHECK_WARNING_THRESHOLD_PERCENT) {
            LOG_WRN("[QUEUE] warning: backlog reached %u%%", fill_percent);
        }

        k_msleep(HEALTHCHECK_PERIOD_MS);
    }
}

// Message subscriber: logger thread
static void logger_thread_fn(void* p1, void* p2, void* p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int logger_wdt_id = task_wdt_add(LOGGER_WDT_PERIOD_MS, logger_task_wdt_cb, (void*)k_current_get());
    if (logger_wdt_id < 0) {
        LOG_ERR("[LOGGER-WDT] failed to add task watchdog ret=%d", logger_wdt_id);
        return;
    }

    uint8_t received = 0;
    struct motion_state motion = {0};

    while (received < SENSOR_COUNT) {
        struct sensor_data msg;
        struct zbus_channel const* chan;
        int ret = zbus_sub_wait_msg(&logger_sub, &chan, &msg, K_MSEC(1500));
        if (ret != 0) {
            LOG_WRN("[LOGGER-MSG] timeout ret=%d", ret);
            break;
        }

        received++;

        struct vector3 displacement = integrate_motion(&motion, &msg.acceleration_mms2,
                                                       msg.timestamp_ms);

        LOG_INF("[LOGGER-MSG] seq=%u accel=(%d, %d, %d) displacement=%u mm "
                "position=%u mm speed=%u mm/s latency=%u ms",
                msg.seq,
                msg.acceleration_mms2.x,
                msg.acceleration_mms2.y,
                msg.acceleration_mms2.z,
                vector_magnitude(&displacement),
                vector_magnitude(&motion.position_mm),
                vector_magnitude(&motion.velocity_mms),
                k_uptime_get_32() - msg.timestamp_ms);

        task_wdt_feed(logger_wdt_id);

        // Simulate a consumer stall so the queue fills and the watchdog callback fires
        if (received == LOGGER_STALL_AFTER_RECEIVED) {
            LOG_WRN("[LOGGER-MSG] simulating stuck consumer after %d messages for %d ms",
                    LOGGER_STALL_AFTER_RECEIVED, LOGGER_STALL_MS);
            k_msleep(LOGGER_STALL_MS);
        }

        // Message copies are queued while the logger processes them more slowly.
        k_msleep(LOGGER_PERIOD_MS);
    }

    LOG_INF("[LOGGER-MSG] done received=%d", received);
}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(healthcheck_thread, STACK_SIZE, healthcheck_thread_fn, NULL, NULL, NULL, 4, 0, 0);

int main(void) {
    const struct device* hw_wdt_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(watchdog0));
    int ret = task_wdt_init(hw_wdt_dev);

    if (ret != 0) {
        LOG_ERR("[MAIN] task_wdt_init failed ret=%d", ret);
        return 0;
    }

    LOG_INF("=== Zbus Example ===");
    LOG_INF("sensor publishes every %d ms", SENSOR_PERIOD_MS);
    LOG_INF("display listener runs in publisher context");
    LOG_INF("logger uses message subscriber copies");

    return 0;
}