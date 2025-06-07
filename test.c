#include <stdio.h>
#include "uthreads.h"
#include "queue.h"
typedef unsigned long address_t;
#define JB_PC 7

extern void uthread_reset_threads_except_main(void);
extern int uthread_get_tid_of_current_internal(void);

static int g_count_tests=0;
static int g_count_passed_tests=0;


void self_terminate_entry() {
    int my_tid = uthread_get_tid_of_current_internal();
    uthread_terminate(my_tid);
}

void print_summary() {
    printf("=========== TEST SUMMARY ===========\n");
    printf("Total tests: %d\n", g_count_tests);
    printf("Passed tests: %d\n", g_count_passed_tests);
    printf("Failed tests: %d\n", g_count_tests - g_count_passed_tests);
    printf("====================================\n");
}


// Dummy function used as thread entry point
void dummy_thread() {
    printf("This should not be printed if thread hasn't run.\n");
}
void simple_entry() {
    while (1) { }  // Infinite loop dummy
}



void reset_threads_except_main() {
    uthread_reset_threads_except_main();
}

void test_init_with_invalid_quantum() {
    g_count_tests++;
    printf("Running test_init_with_invalid_quantum...\n");

    int result = uthread_init(0); // invalid input
    if (result != -1) {
        printf("[✗] test_init_with_invalid_quantum failed: expected -1, got %d\n", result);
    } else {
        g_count_passed_tests++;
        printf("[✓] test_init_with_invalid_quantum passed.\n");
    }
}

void test_main_thread_initialized_correctly() {
    printf("Running test_main_thread_initialized_correctly...\n");
    g_count_tests++;


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
    g_count_passed_tests++;

}


// Test 1: Basic spawning and state check
void test_basic_spawn() {
    printf("Running test_basic_spawn...\n");
    g_count_tests++;


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
    g_count_passed_tests++;
}

// Test 2: Check if threads are in the READY queue in correct order
void test_ready_queue_integrity() {
    printf("Running test_ready_queue_integrity...\n");
    g_count_tests++;


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
    g_count_passed_tests++;
}
void test_block_valid_thread() {
    printf("Running test_block_valid_thread...\n");
    g_count_tests++;


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
    g_count_passed_tests++;
}

void test_block_main_thread() {
    printf("Running test_block_main_thread...\n");
    g_count_tests++;


    int result = uthread_block(0);  // Try to block main thread
    if (result != -1) {
        printf("[✗] test_block_main_thread failed: should not block main thread.\n");
        return;
    }

    printf("[✓] test_block_main_thread passed.\n");
    g_count_passed_tests++;
}
void test_ready_queue_clean_after_terminate() {
    printf("Running test_ready_queue_clean_after_terminate...\n");
    g_count_tests++;


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
        g_count_passed_tests++;
    } else {
        printf("[✗] Ready queue is NOT empty after terminating all threads!\n");
    }
}

void test_thread_terminates_itself() {
    printf("Running test_thread_terminates_itself...\n");
    g_count_tests++;

    uthread_init(100000);

    int tid = uthread_spawn(self_terminate_entry);
    printf("Spawned self-terminate thread: %d\n", tid);
    raise(SIGVTALRM);


    // Wait a little to allow the thread to run and terminate itself
    usleep(200000);

    if (ready_queue_is_empty()) {
        printf("[✓] Self-terminating thread successfully terminated and ready queue is empty.\n");
        g_count_passed_tests++;
    } else {
        printf("[✗] Self-terminating thread did not clean properly!\n");
    }
}void thread_get_tid_entry() {
    int my_tid = uthread_get_tid();
    if (my_tid != uthread_get_tid_of_current_internal()) {
        printf("[✗] test_get_tid failed inside thread: expected %d, got %d\n",
               uthread_get_tid_of_current_internal(), my_tid);
    } else {
        printf("[✓] test_get_tid passed inside thread.\n");
        g_count_passed_tests++;
    }
    // Terminate self
    int self_tid = uthread_get_tid();
    uthread_terminate(self_tid);
}

void test_get_tid() {
    printf("Running test_get_tid...\n");
    g_count_tests++;

    uthread_init(100000);

    int main_tid = uthread_get_tid();
    if (main_tid != 0) {
        printf("[✗] test_get_tid failed: expected 0, got %d\n", main_tid);
        return;
    } else {
        printf("[✓] test_get_tid passed in main thread.\n");
        g_count_passed_tests++;
    }

    // Now test inside another thread:
    int tid1 = uthread_spawn(thread_get_tid_entry);
    if (tid1 == -1) {
        printf("[✗] test_get_tid failed: spawn failed.\n");
        return;
    }

    raise(SIGVTALRM);  // Allow thread to run
}
void test_get_total_quantums() {
    printf("Running test_get_total_quantums...\n");
    g_count_tests++;

    uthread_init(100000);
    int initial_quantums = uthread_get_total_quantums();
    if (initial_quantums != 1) {
        printf("[✗] test_get_total_quantums failed: expected 1, got %d\n", initial_quantums);
        return;
    }

    raise(SIGVTALRM);
    int after_first_tick = uthread_get_total_quantums();
    if (after_first_tick <= initial_quantums) {
        printf("[✗] test_get_total_quantums failed: quantums did not increase.\n");
    } else {
        printf("[✓] test_get_total_quantums passed.\n");
        g_count_passed_tests++;

    }
}
void test_get_quantums() {
    printf("Running test_get_quantums...\n");
    g_count_tests++;

    uthread_init(100000);

    int tid1 = uthread_spawn(dummy_thread);
    if (tid1 == -1) {
        printf("[✗] test_get_quantums failed: spawn failed.\n");
        return;
    }

    // Initially thread should have 0 quantums
    int q = uthread_get_quantums(tid1);
    if (q != 0) {
        printf("[✗] test_get_quantums failed: expected 0, got %d\n", q);
        return;
    }

    // Force context switch
    raise(SIGVTALRM);

    // Now main thread should have at least 1 quantum
    int main_q = uthread_get_quantums(0);
    if (main_q <= 0) {
        printf("[✗] test_get_quantums failed: main thread quantums invalid.\n");
    } else {
        printf("[✓] test_get_quantums passed.\n");
        g_count_passed_tests++;

    }
}
static jmp_buf main_env;
static int context_switch_test_flag = 0;

void dummy_context_switch_target() {
    // סימולציה של פעולה בתוך ה־dummy thread
    context_switch_test_flag = 1;
    siglongjmp(main_env, 1);  // חזרה ל־main
}

void test_context_switch() {
    printf("Running test_context_switch...\n");
    g_count_tests++;

    static int test_context_switch_done = 0;

    if (sigsetjmp(main_env, 1) == 0) {
        // First pass
        thread_t dummy_thread;
        sigsetjmp(dummy_thread.env, 1);
        dummy_thread.env->__jmpbuf[JB_PC] = (address_t)dummy_context_switch_target;

        thread_t dummy_main;
        sigsetjmp(dummy_main.env, 1);

        context_switch(&dummy_main, &dummy_thread);
    } else {
        // Returned from dummy_context_switch_target
        if (context_switch_test_flag == 1) {
            printf("[✓] test_context_switch passed.\n");
            g_count_passed_tests++;
        } else {
            printf("[✗] test_context_switch failed.\n");
        }

        // prevent re-entry to context_switch infinite loop!
        test_context_switch_done = 1;
    }

    if (test_context_switch_done) {
        // Disable timer to prevent stray SIGVTALRM
        struct itimerval stop_timer = {0};
        setitimer(ITIMER_VIRTUAL, &stop_timer, NULL);

        // Also block SIGVTALRM temporarily to clear any pending signal
        sigset_t block_mask;
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGVTALRM);
        sigprocmask(SIG_BLOCK, &block_mask, NULL);

        // Small sleep to ensure the kernel clears the pending signal
        usleep(10000);  // 10 ms is more than enough

        // Unblock SIGVTALRM
        sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

        return;  // Exit the test cleanly
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
    test_get_tid();
    test_get_total_quantums();
    test_get_quantums();
    test_context_switch();
    print_summary();
    return 0;
}
