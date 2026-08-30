#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_INF);

#define STACK_SIZE 1024

static const int PRIO = 5;

// Each thread increments this many times
static const uint32_t INCREMENTS = 500000;

// Shared state
static volatile uint32_t counter;

static struct k_sem done_sem;

static K_MUTEX_DEFINE(counter_mutex);

void worker_fn(void* p1, void* p2, void* p3) {
    for (int i = 0; i < INCREMENTS; i++) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        counter++;

        if (counter % 100000 == 0) {
            LOG_INF("Counter: %u", counter);
        }
        k_mutex_unlock(&counter_mutex);
    }

    const char* name = k_thread_name_get(k_current_get());
    LOG_INF("[%s] finished", name);
    k_sem_give(&done_sem);
}

K_THREAD_DEFINE(worker_a, STACK_SIZE, worker_fn, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(worker_b, STACK_SIZE, worker_fn, NULL, NULL, NULL, PRIO, 0, 0);

int main(void) {
    k_sem_init(&done_sem, 0, 2);
    int64_t time = k_uptime_get();

    LOG_INF("=== Mutex Protection Demo ===");
    LOG_INF("Expected final value: %d", INCREMENTS * 2);

    // Wait for both workers to complete
    k_sem_take(&done_sem, K_FOREVER);
    k_sem_take(&done_sem, K_FOREVER);

    LOG_INF("Actual final value: %u", counter);

    if (counter == (INCREMENTS * 2)) {
        LOG_WRN("No race!");
    } else {
        LOG_ERR("Race condition confirmed: lost %d updates", (INCREMENTS * 2) - counter);
    }
    LOG_INF("Execution time: %lld ms", k_uptime_delta(&time));

    return 0;
}