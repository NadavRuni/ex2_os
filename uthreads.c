#include "uthreads.h"
#include "queue.h"

#define _GNU_SOURCE
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_THREAD_NUM 100
#define STACK_SIZE 4096
#define JB_SP 6
#define JB_PC 7


typedef unsigned long address_t;

static char stacks[MAX_THREAD_NUM][STACK_SIZE];
static thread_t g_thread_table[MAX_THREAD_NUM]; // Array of thread control blocks (TCBs).

static int g_current_running_tid = -1; // bolloean
static int g_total_quantums_elapsed = 0; // Total number of quantums since initialization.
static int g_queantum_duration_usecs = 0; // Duration of each quantum in microseconds.


static void system_error_exit(const char* msg);
static void library_error_print(const char* msg);



//static void thread_wrapper(void);
void schedule_next(void);
void timer_handler(int signum);
// Declare it here locally so test.c knows it exists:
extern void uthread_reset_threads_except_main(void);
int uthread_get_tid_of_current_internal(void) {
    return g_current_running_tid;
}



//=====================================================================//=====================================================================
static void system_error_exit(const char* msg) {
    fprintf(stderr, "system error: %s\n", msg);
    exit(1);
}
void uthread_reset_threads_except_main() {
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


static void library_error_print(const char* msg) {
    fprintf(stderr, "thread library error: %s\n", msg);
}

//=====================================================================//=====================================================================
//=====================================================================//=====================================================================
  void timer_handler(int signum) {

    g_total_quantums_elapsed++;
    if (g_current_running_tid != -1 && g_thread_table[g_current_running_tid].state == THREAD_RUNNING) {
        g_thread_table[g_current_running_tid].quantums++;
    }
    schedule_next();
  }
void schedule_next(void) {
    int prev_tid = g_current_running_tid;

    // Save previous thread if still valid and not TERMINATED
    if (prev_tid != -1 && g_thread_table[prev_tid].state == THREAD_RUNNING) {
        if (g_thread_table[prev_tid].state != THREAD_TERMINATED && g_thread_table[prev_tid].state != THREAD_BLOCKED &&  prev_tid != 0)  {
            g_thread_table[prev_tid].state = THREAD_READY;
            ready_queue_push(prev_tid);
        }
    }

    // Pick next thread
    int next_tid = ready_queue_pop();

    // Fallback to main thread if needed
    if (next_tid == -1) {
        if (g_thread_table[0].state == THREAD_READY || g_thread_table[0].state == THREAD_RUNNING) {
            next_tid = 0;
        } else {
            system_error_exit("schedule_next: No runnable threads (critical error).");
        }
    }

    g_current_running_tid = next_tid;
    g_thread_table[next_tid].state = THREAD_RUNNING;

    // Reset timer
    struct itimerval timer_reset_val;
    timer_reset_val.it_value.tv_sec = g_queantum_duration_usecs / 1000000;
    timer_reset_val.it_value.tv_usec = g_queantum_duration_usecs % 1000000;
    timer_reset_val.it_interval.tv_sec = g_queantum_duration_usecs / 1000000;
    timer_reset_val.it_interval.tv_usec = g_queantum_duration_usecs % 1000000;

    if (setitimer(ITIMER_VIRTUAL, &timer_reset_val, NULL) == -1) {
        system_error_exit("schedule_next: setitimer failed to reset quantum.");
    }
}


//=====================================================================//=====================================================================
address_t translate_address(address_t addr) {
    return addr;
}
thread_t* get_thread(int tid) {
    return &g_thread_table[tid];
}

int uthread_spawn(thread_entry_point entry_point) {
    if (entry_point == NULL) {
        fprintf(stderr, "thread library error: entry_point is NULL\n");
        return -1;
    }

    // מוצא את המקום הפנוי במערך ושומר את האינדקס שלו ב tid
    int tid = -1;
    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        if (g_thread_table[i].state == THREAD_UNUSED) {
            tid = i;
            break;
        }
    }
    // אם אין מקום פנוי בכלל
    if (tid == -1) {
        fprintf(stderr, "thread library error: exceeded max thread count\n");
        return -1;
    }

    // מאתחל את הthread החדש

    //index
    g_thread_table[tid].tid = tid;
    // state
    g_thread_table[tid].state = THREAD_READY;
    // quantums counter
    g_thread_table[tid].quantums = 0;
    //time of sleep (beacuse its new , no sleep)
    g_thread_table[tid].sleep_until = 0;
    //function
    g_thread_table[tid].entry = entry_point;

    //  נשמור את הערכים של sigsetjmp ֿ
    // את מצב הרגיסטרים של הcpu לתוך env
    // בגלל שזה שמירה ראשוונה אז היא תחזיר 0
    if (sigsetjmp(g_thread_table[tid].env, 1) == 0) {
        // מחשבים את הפוינטר של המחסנית מחדש
        address_t sp = (address_t)stacks[tid] + STACK_SIZE - sizeof(address_t);
        // קובעים את המקום החדש שממנה הthread יתחיל לרוץ
        address_t pc = (address_t)entry_point;

        // Set the stack and program counter
        // This assumes x86_64 or compatible
        g_thread_table[tid].env->__jmpbuf[JB_SP] = translate_address(sp);
        g_thread_table[tid].env->__jmpbuf[JB_PC] = translate_address(pc);
        sigemptyset(&g_thread_table[tid].env->__saved_mask);

        // Add to READY queue
        ready_queue_push(tid);
        return tid;
    }

    // Never reached in the parent
    return -1;
}
int uthread_block(int tid) {
    //check if the id is valid
    if (tid < 0 || tid >= MAX_THREAD_NUM) {
        return -1;
    }

    //pointer to the thread
    thread_t* t = &g_thread_table[tid];

    //check if the thread is not active
    if (t->state == THREAD_UNUSED || t->state == THREAD_TERMINATED) {
        return -1;
    }

    //dont stop main thread
    if (tid == 0) {
        return -1;
    }

    //if its block dont do nothing
    if (t->state == THREAD_BLOCKED) {
        return 0;
    }

    //if we are here block it
    t->state = THREAD_BLOCKED;
    return 0;
}

int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) {
        library_error_print("Quantum duration must be positive.");
        return -1;
    }

    g_queantum_duration_usecs = quantum_usecs;

    sigset_t old_mask, block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGVTALRM);
    if(sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
        system_error_exit("uthread_init: sigprocmask SIG_BLOCK failed.");
    }

    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        g_thread_table[i].tid = -1; // Mark all TCBs as unused.
        g_thread_table[i].state = THREAD_UNUSED;
        g_thread_table[i].quantums = 0;
        g_thread_table[i].sleep_until = 0;
        g_thread_table[i].entry = NULL; // No entry function for unused threads.
    }

    g_total_quantums_elapsed = 1; // Start counting quantums from 1.
    g_thread_table[0].tid = 0; // Main thread ID is 0.
    g_thread_table[0].state = THREAD_RUNNING;
    g_thread_table[0].quantums = 0; // Main thread ID is 0.
    g_current_running_tid = 0;


    struct sigaction sa;
    sa.sa_handler = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGVTALRM);
    sa.sa_flags = 0;

    if(sigaction(SIGVTALRM, &sa, NULL) == -1) {
        sigprocmask(SIG_SETMASK, &old_mask, NULL); // Restore old signal mask
        system_error_exit("uthread_init: sigaction failed.");
    }

    struct itimerval timer_val;
    timer_val.it_value.tv_sec = g_queantum_duration_usecs / 1000000;
    timer_val.it_value.tv_usec = g_queantum_duration_usecs % 1000000;
    timer_val.it_interval.tv_sec = g_queantum_duration_usecs / 1000000;
    timer_val.it_interval.tv_usec = g_queantum_duration_usecs % 1000000;

    if(setitimer(ITIMER_VIRTUAL, &timer_val, NULL) == -1) {
        sigprocmask(SIG_SETMASK, &old_mask, NULL); // Restore old signal mask
        system_error_exit("uthread_init: setitimer failed.");
        return -1;
    }

    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
        system_error_exit("uthread_init: sigprocmask failed.");
        return -1;
    }

   return 0;
  }
int uthread_terminate(int tid) {
    if (tid<0 || tid >= MAX_THREAD_NUM) {
        return -1;
    }
    if (tid==0) {
        for (int i = 0; i < MAX_THREAD_NUM; i++) {
            if (g_thread_table[i].state != THREAD_UNUSED && g_thread_table[i].state != THREAD_TERMINATED) {
                ready_queue_remove(i);
                g_thread_table[i].state = THREAD_TERMINATED;
                g_thread_table[i].tid = -1;
                g_thread_table[i].entry = NULL;
                g_thread_table[i].quantums = 0;
            }

        }
        // Clean main thread
        g_thread_table[0].state = THREAD_TERMINATED;
        g_thread_table[0].tid = -1;
        g_thread_table[0].entry = NULL;
        g_thread_table[0].quantums = 0;
        exit(0);
    }
    else {
        if (tid == g_current_running_tid) {
            // If self-terminating
            g_thread_table[tid].state = THREAD_TERMINATED;
            g_thread_table[tid].tid = -1;
            g_thread_table[tid].entry = NULL;
            g_thread_table[tid].quantums = 0;

            // No need to remove from ready_queue because RUNNING thread is not in the ready queue!
            g_current_running_tid = -1;
            raise(SIGVTALRM); // Force context switch → no return
        } else {
            // Normal case: terminate other thread
            ready_queue_remove(tid);

            g_thread_table[tid].state = THREAD_TERMINATED;
            g_thread_table[tid].tid = -1;
            g_thread_table[tid].entry = NULL;
            g_thread_table[tid].quantums = 0;

            return 0; // Success
        }
    }
}
