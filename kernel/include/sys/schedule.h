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

