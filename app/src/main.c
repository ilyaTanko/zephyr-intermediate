// Event-driven workqueue architecture.

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_DBG);

#define STACK_SIZE    1024

// sensor fires every 100ms
static const int SENSOR_MS = 100;
static const int BURST_DURATION_MS = 20;
static const int REST_MS = SENSOR_MS - BURST_DURATION_MS;

static const int BOUNCES_IN_BURST = 5;
static const int BOUNCE_DURATION_MS = BURST_DURATION_MS / BOUNCES_IN_BURST;

// total sensor events to produce
static const int EVENT_COUNT = 10;

// Statistics
static int total_events;
static int total_processed;

 static void sensor_handler(struct k_work* work) {
    ARG_UNUSED(work);

    total_processed++;
    LOG_INF("[HANDLER] processed event %d  tick=%u", total_processed, k_uptime_get_32());
}

K_WORK_DELAYABLE_DEFINE(debounce_work, sensor_handler);

static void sensor_sim_fn(void* p1, void* p2, void* p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    // Fires EVENT_COUNT events, SENSOR_MS apart
    for (int i = 0; i < EVENT_COUNT; i++) {
        total_events++;
        LOG_INF("[SENSOR] event %d  tick=%u", total_events, k_uptime_get_32());

        for (int j = 0; j < BOUNCES_IN_BURST; j++) {
            k_msleep(BOUNCE_DURATION_MS);
            LOG_INF("[SENSOR] bounce %d  tick=%u", j + 1, k_uptime_get_32());
            int ret = k_work_reschedule(&debounce_work, K_MSEC(30));
            if (ret < 0) {
                LOG_ERR("reschedule failed: %d", ret);
            }
        }
        k_msleep(REST_MS);
    }

    LOG_INF("[SENSOR] all events produced");
}

K_THREAD_DEFINE(sensor_thread,  STACK_SIZE, sensor_sim_fn, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    LOG_INF("=== L3 Homework: Workqueue ===");

    // Wait long enough for all events to complete
    k_msleep((EVENT_COUNT + 2) * SENSOR_MS + 500);

    LOG_INF("Events processed: %d", total_processed);

    return 0;
}