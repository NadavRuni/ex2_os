#include <stdio.h>
#include "queue.h"

#define MAX_THREAD_NUM 100

static int g_ready_queue[MAX_THREAD_NUM]; // Queue holding TIDs
static int g_ready_queue_head = 0;
static int g_ready_queue_tail = 0;
static int g_ready_queue_size = 0;

void ready_queue_push(int tid) {
    if (g_ready_queue_size >= MAX_THREAD_NUM) {
        fprintf(stderr, "ready_queue error: overflow\n");
        return;
    }
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
    int size = ready_queue_size();
    int temp_queue[MAX_THREAD_NUM];
    int count = 0;

    while (size > 0) {
        int curr_tid = ready_queue_pop();
        if (curr_tid != tid) {
            temp_queue[count++] = curr_tid;
        }
    }

    for (int i = 0; i < count; i++) {
        ready_queue_push(temp_queue[i]);
    }
}
int ready_queue_is_empty() {
    return g_ready_queue_size == 0;
}

