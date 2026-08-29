#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

static const int PRIO_COOP = -1;
static const int PRIO_HIGH = 3;
static const int PRIO_MEDIUM = 5;
static const int PRIO_LOW = 7;

void t_low_fn(void* p1, void* p2, void* p3) {
    LOG_INF("Thread Low started");
    static int count = 0;

    while (true) {
        LOG_INF("Thread Low count: %d, tick: %u", count++, k_uptime_get_32());
        k_msleep(300);
    }
}

void t_medium_fn(void* p1, void* p2, void* p3) {
    LOG_INF("Thread Medium started");
    static int count = 0;

    while (true) {
        LOG_INF("Thread Medium count: %d, tick: %u", count++, k_uptime_get_32());
        k_msleep(200);
    }
}

void t_high_fn(void* p1, void* p2, void* p3) {
    LOG_INF("Thread High started");
    static int count = 0;

    while (true) {
        LOG_INF("Thread High count: %d, tick: %u", count++, k_uptime_get_32());
        k_msleep(100);
    }
}

void t_coop_fn(void* p1, void* p2, void* p3) {
    LOG_INF("Thread Coop started");

    while (true) {
        static const uint8_t ITERATIONS = 5;
        for (int i = 0; i < ITERATIONS; i++) {
            k_busy_wait(500 * 1000); // 500 ms
            LOG_INF("Thread Coop step %d/%d tick=%u", i + 1, ITERATIONS, k_uptime_get_32());
        }
        k_yield();
    }
}

K_THREAD_DEFINE(thread_low, STACK_SIZE, t_low_fn, NULL, NULL, NULL, PRIO_LOW, 0, 0);
K_THREAD_DEFINE(thread_medium, STACK_SIZE, t_medium_fn, NULL, NULL, NULL, PRIO_MEDIUM, 0, 0);
K_THREAD_DEFINE(thread_high, STACK_SIZE, t_high_fn, NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(thread_coop, STACK_SIZE, t_coop_fn, NULL, NULL, NULL, PRIO_COOP, 0, 0);

int main(void) {
    return 0;
}

