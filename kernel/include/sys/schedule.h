#pragma once

// Core includes for timekeeping, synchronization, and data structures
#include <core/jiffies.h>
#include <core/spinlock.h>
#include <ds/queue.h>

// Definition of a scheduling level in the MLFQ
typedef struct {
    queue_t     queue;      // Queue to hold processes for this level
    jiffies_t   quantum;    // Time quantum for this level
    spinlock_t  lock;       // Lock to ensure thread-safe operations
} sched_level_t;

// Number of scheduling levels in the MLFQ
#define NSCHED_LEVEL 4

// Definition of the Multi-Level Feedback Queue (MLFQ) structure
typedef struct {
    sched_level_t   level[NSCHED_LEVEL]; // Array of scheduling levels
} mlfq_t;

// Highest priority level.
#define MLFQ_HIGHEST    (NSCHED_LEVEL - 1)

// Lowest priority level.
#define MLFQ_LOWEST     0

extern const char *MLFQ_PRIORITY[];

int scheduler_init(void);

__noreturn void scheduler(void);

typedef struct {
    uint64_t total_context_switches;
    uint64_t total_cpu_time;       // Total CPU time across all threads.
    uint64_t total_wait_time;      // Total wait time for all threads.
    uint64_t total_threads_executed;
    uint64_t idle_time;            // Time CPU spent idle.
    uint64_t preemption_count;
    uint64_t steal_attempts;
    uint64_t successful_steals;
} sched_metrics_t;
