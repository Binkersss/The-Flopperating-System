#include "mutex.h"
#include "spinlock.h"
#include "../sched.h"
#include <stdint.h>

typedef struct turnstile {
    mutex_t lock;
    thread_list_t wait_queue;
    spinlock_t wait_lock;
} turnstile_t;

void turnstile_init(turnstile_t* turnstile) {
    mutex_init(&turnstile->lock, "turnstile");
    turnstile->wait_queue.head = NULL;
    turnstile->wait_queue.tail = NULL;
    turnstile->wait_queue.count = 0;
    spinlock_init(&turnstile->wait_queue.lock);
    spinlock_init(&turnstile->wait_lock);
}

void turnstile_enter(turnstile_t* turnstile, process_t* owner) {
    mutex_lock(&turnstile->lock, owner);
}

void turnstile_exit(turnstile_t* turnstile) {
    mutex_unlock(&turnstile->lock);
}

void turnstile_wait(turnstile_t* turnstile) {
    thread_t* cur = current_thread;
    spinlock(&turnstile->wait_lock);
    thread_t* prev = NULL;
    thread_t* iter = turnstile->wait_queue.head;

    while (iter && iter->priority.effective >= cur->priority.effective) {
        prev = iter;
        iter = iter->next;
    }

    if (!prev) {
        cur->next = turnstile->wait_queue.head;
        turnstile->wait_queue.head = cur;
        if (!turnstile->wait_queue.tail) {
            turnstile->wait_queue.tail = cur;
        }
    } else {
        cur->next = prev->next;
        prev->next = cur;
        if (!cur->next) {
            turnstile->wait_queue.tail = cur;
        }
    }

    turnstile->wait_queue.count++;
    spinlock_unlock(&turnstile->wait_lock, true);
    sched_block();
}

void turnstile_wake(turnstile_t* turnstile) {
    spinlock(&turnstile->wait_lock);
    thread_t* highest = turnstile->wait_queue.head;
    thread_t* prev = NULL;
    thread_t* iter = turnstile->wait_queue.head;
    thread_t* prev_high = NULL;

    while (iter) {
        if (iter->priority.effective > highest->priority.effective) {
            highest = iter;
            prev_high = prev;
        }
        prev = iter;
        iter = iter->next;
    }

    if (highest) {
        if (prev_high) {
            prev_high->next = highest->next;
        } else {
            turnstile->wait_queue.head = highest->next;
        }

        if (highest == turnstile->wait_queue.tail) {
            turnstile->wait_queue.tail = prev_high;
        }

        highest->next = NULL;
        turnstile->wait_queue.count--;
        sched_unblock(highest);
    }

    spinlock_unlock(&turnstile->wait_lock, true);
}

void turnstile_wake_all(turnstile_t* turnstile) {
    spinlock(&turnstile->wait_lock);
    while (turnstile->wait_queue.head) {
        turnstile_wake(turnstile);
    }
    spinlock_unlock(&turnstile->wait_lock, true);
}

int turnstile_empty(turnstile_t* turnstile) {
    spinlock(&turnstile->wait_lock);
    int empty = turnstile->wait_queue.count == 0;
    spinlock_unlock(&turnstile->wait_lock, true);
    return empty;
}
