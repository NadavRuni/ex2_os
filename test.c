#include <stdio.h>
#include "uthreads.h"
#include "queue.h"

// Dummy function used as thread entry point
void dummy_thread() {
    printf("This should not be printed if thread hasn't run.\n");
}
void reset_threads_except_main() {
    while (!ready_queue_is_empty()) {
        ready_queue_pop();
    }

    for (int i = 1; i < MAX_THREAD_NUM; i++) {
        g_thread_table[i].tid = -1;
        g_thread_table[i].state = THREAD_UNUSED;
        g_thread_table[i].quantums = 0;
        g_thread_table[i].sleep_until = 0;
        g_thread_table[i].entry = NULL;
    }
}

void test_init_with_invalid_quantum() {
    printf("Running test_init_with_invalid_quantum...\n");

    int result = uthread_init(0); // invalid input
    if (result != -1) {
        printf("[✗] test_init_with_invalid_quantum failed: expected -1, got %d\n", result);
    } else {
        printf("[✓] test_init_with_invalid_quantum passed.\n");
    }
}

void test_main_thread_initialized_correctly() {
    printf("Running test_main_thread_initialized_correctly...\n");

    if (uthread_init(100000) == -1) {
        printf("[✗] test_main_thread_initialized_correctly failed: init failed.\n");
        return;
    }

    thread_t* main_thread = get_thread(0);
    if (main_thread->tid != 0 || main_thread->state != THREAD_RUNNING || main_thread->quantums != 0) {
        printf("[✗] test_main_thread_initialized_correctly failed: incorrect main thread init.\n");
        return;
    }

    printf("[✓] test_main_thread_initialized_correctly passed.\n");
}


// Test 1: Basic spawning and state check
void test_basic_spawn() {
    printf("Running test_basic_spawn...\n");

    int tid1 = uthread_spawn(dummy_thread);
    int tid2 = uthread_spawn(dummy_thread);

    if (tid1 == -1 || tid2 == -1) {
        printf("[✗] test_basic_spawn failed: failed to spawn threads.\n");
        return;
    }

    if (get_thread(tid1)->state != THREAD_READY || get_thread(tid2)->state != THREAD_READY) {
        printf("[✗] test_basic_spawn failed: threads not in READY state.\n");
        return;
    }

    printf("[✓] test_basic_spawn passed.\n");
}

// Test 2: Check if threads are in the READY queue in correct order
void test_ready_queue_integrity() {
    printf("Running test_ready_queue_integrity...\n");

    const int num_extra_threads = 5;
    int tids[num_extra_threads];

    // ניקוי ה־queue מכל תהליך קודם
    while (!ready_queue_is_empty()) {
        ready_queue_pop();
    }

    // Spawn extra threads
    for (int i = 0; i < num_extra_threads; ++i) {
        tids[i] = uthread_spawn(dummy_thread);
        if (tids[i] == -1) {
            printf("[✗] test_ready_queue_integrity failed: spawn #%d failed.\n", i);
            return;
        }

        if (get_thread(tids[i])->state != THREAD_READY) {
            printf("[✗] test_ready_queue_integrity failed: thread %d not in READY state.\n", tids[i]);
            return;
        }
    }

    // Check queue order
    for (int i = 0; i < num_extra_threads; ++i) {
        int popped_tid = ready_queue_pop();
        if (popped_tid != tids[i]) {
            printf("[✗] test_ready_queue_integrity failed: expected %d, got %d\n", tids[i], popped_tid);
            return;
        }
    }

    printf("[✓] test_ready_queue_integrity passed.\n");
}
void test_block_valid_thread() {
    printf("Running test_block_valid_thread...\n");

    // ננקה את ה־queue
    while (!ready_queue_is_empty()) {
        ready_queue_pop();
    }

    int tid = uthread_spawn(dummy_thread);
    if (tid == -1) {
        printf("[✗] test_block_valid_thread failed: spawn failed.\n");
        return;
    }

    if (get_thread(tid)->state != THREAD_READY) {
        printf("[✗] test_block_valid_thread failed: not in READY state.\n");
        return;
    }

    int result = uthread_block(tid);
    if (result != 0) {
        printf("[✗] test_block_valid_thread failed: block returned error.\n");
        return;
    }

    if (get_thread(tid)->state != THREAD_BLOCKED) {
        printf("[✗] test_block_valid_thread failed: state not updated to BLOCKED.\n");
        return;
    }

    printf("[✓] test_block_valid_thread passed.\n");
}

void test_block_main_thread() {
    printf("Running test_block_main_thread...\n");

    int result = uthread_block(0);  // Try to block main thread
    if (result != -1) {
        printf("[✗] test_block_main_thread failed: should not block main thread.\n");
        return;
    }

    printf("[✓] test_block_main_thread passed.\n");
}
void test_ready_queue_clean_after_terminate() {
    printf("Running test_ready_queue_clean_after_terminate...\n");

    reset_threads_except_main();
    int tid1 = uthread_spawn(simple_entry);
    int tid2 = uthread_spawn(simple_entry);
    int tid3 = uthread_spawn(simple_entry);

    printf("Spawned threads: %d, %d, %d\n", tid1, tid2, tid3);

    uthread_terminate(tid1);
    uthread_terminate(tid2);
    uthread_terminate(tid3);

    if (ready_queue_is_empty()) {
        printf("[✓] Ready queue is empty after terminating all threads.\n");
    } else {
        printf("[✗] Ready queue is NOT empty after terminating all threads!\n");
    }
}

void test_thread_terminates_itself() {
    printf("Running test_thread_terminates_itself...\n");

    uthread_init(100000);

    int tid = uthread_spawn(self_terminate_entry);
    printf("Spawned self-terminate thread: %d\n", tid);

    // Wait a little to allow the thread to run and terminate itself
    usleep(200000);

    if (ready_queue_is_empty()) {
        printf("[✓] Self-terminating thread successfully terminated and ready queue is empty.\n");
    } else {
        printf("[✗] Self-terminating thread did not clean properly!\n");
    }
}







int main() {
    // Initialize threading system
    // if (uthread_init(100000) == -1) {
    //     fprintf(stderr, "uthread_init failed.\n");
    //     return 1;
    // }

    test_init_with_invalid_quantum();
    test_main_thread_initialized_correctly();
    test_basic_spawn();
    test_ready_queue_integrity();
    test_block_valid_thread();
    test_block_main_thread();
    test_ready_queue_clean_after_terminate();
    test_thread_terminates_itself();


    return 0;
}
