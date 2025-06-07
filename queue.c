#include <stdio.h>
#include "queue.h"

#define MAX_THREAD_NUM 100

static int g_ready_queue[MAX_THREAD_NUM]; // Queue holding TIDs
static int g_ready_queue_head = 0;
static int g_ready_queue_tail = 0;
static int g_ready_queue_size = 0;

void ready_queue_push(int tid) {
    if (g_ready_queue_size >= MAX_THREAD_NUM) {
        fprintf(stderr, "ready_queue error: overflow when pushing %d\n", tid);
        return;
    }
    printf("[DEBUG] ready_queue_push(%d)\n", tid);  // הוספת הדפסה

    g_ready_queue[g_ready_queue_tail] = tid;
    g_ready_queue_tail = (g_ready_queue_tail + 1) % MAX_THREAD_NUM;
    g_ready_queue_size++;
}

int ready_queue_pop() {
    if (g_ready_queue_size == 0) {
        return -1;
    }
    int tid = g_ready_queue[g_ready_queue_head];
    g_ready_queue_head = (g_ready_queue_head + 1) % MAX_THREAD_NUM;
    g_ready_queue_size--;
    return tid;
}

int ready_queue_size() {
    return g_ready_queue_size;
}
void ready_queue_remove(int tid) {
    printf("[DEBUG] ready_queue_remove(%d)\n", tid);  // הוספת הדפסה

    int new_queue[MAX_THREAD_NUM];
    int new_head = 0;
    int count = 0;

    while (!ready_queue_is_empty()) {
        int current = ready_queue_pop();
        if (current != tid) {
            new_queue[new_head++] = current;
            count++;
        }
    }

    // Copy back
    for (int i = 0; i < count; i++) {
        ready_queue_push(new_queue[i]);
    }
}

int ready_queue_is_empty() {
    return g_ready_queue_size == 0;
}

