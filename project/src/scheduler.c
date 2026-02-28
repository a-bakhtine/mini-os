#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "scheduler.h"
#include "readyqueue.h"
#include "shellmemory.h"
#include "pcb.h"
#include "interpreter.h"
#include "shell.h"

#define NUM_WORKERS 2

// SCHEDULER STATE 
int scheduler_active = 0; // scheduler being used
Policy scheduler_policy = INVALID;

// MULTITHREADING STATES
int mt_enabled = 0;
int mt_can_run = 0;
int mt_threads_started = 0;
int mt_stop_requested = 0;
int rr_quantum = 2;
int active_workers = 0;

pthread_t worker_threads[NUM_WORKERS];
pthread_mutex_t rq_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t interpreter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rq_has_work = PTHREAD_COND_INITIALIZER;
pthread_cond_t rq_drained = PTHREAD_COND_INITIALIZER;

// SCHEDULER FUNCTIONS
// returns 1 if scheduler running, else 0
int scheduler_is_active() { 
    return scheduler_active; 
}

// returns the currently active scheduling policy
Policy scheduler_get_policy() { 
    return scheduler_policy; 
}

// policy parser: string -> enum
Policy parse_policy(const char *s) {
    if (strcmp(s, "FCFS") == 0) return FCFS;
    if (strcmp(s, "SJF") == 0) return SJF;
    if (strcmp(s, "RR") == 0) return RR;
    if (strcmp(s, "RR30") == 0) return RR30;
    if (strcmp(s, "AGING")== 0) return AGING;
    return INVALID;
}

// MULTITHREADING FUNCTIONS
// enables multithreaded scheduling mode
void scheduler_mt_enable(void) { 
    mt_enabled = 1; 
}

// returns 1 if multithreaded mode enabled, else 0
int  scheduler_mt_enabled(void) { 
    return mt_enabled; 
}

int scheduler_mt_is_worker_thread(void) {
    if (!mt_threads_started) return 0;
    pthread_t self = pthread_self();
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (pthread_equal(self, worker_threads[i])) return 1;
    }
    return 0;
}

// ready queue (thread safe wrappers)
// init RQ
void scheduler_rq_init() {
    pthread_mutex_lock(&rq_mutex);
    rq_init();
    pthread_mutex_unlock(&rq_mutex);
}

// enqueues a process @ back of the RQ and wakes workers
void scheduler_rq_enqueue(PCB *p) {
    pthread_mutex_lock(&rq_mutex);
    rq_enqueue(p);
    pthread_cond_broadcast(&rq_has_work);
    pthread_mutex_unlock(&rq_mutex);
}

// enqueues a proces @ front of the RQ and wakes workers
void scheduler_rq_enqueue_front(PCB *p) {
    pthread_mutex_lock(&rq_mutex);
    rq_enqueue_front(p);
    pthread_cond_broadcast(&rq_has_work);
    pthread_mutex_unlock(&rq_mutex);
}

// SINGLE-THREADED ONLY SCHEDULERS
// run all enqueued process to completion in arrival order
void run_fcfs() {
    int end;
    char *line;
    PCB *p;

    while (!rq_is_empty()) {

        p = rq_dequeue();
        if (p == NULL)
            break;
            
        // end is 1 past last line index
        end = p->start + p->scriptLength;

        while (p->pc < end) {
            line = get_script_line(p->pc);
            
            // skip null lines
            if (line != NULL && strlen(line) > 0) {
                parseInput(line);
            }
            p->pc++;
        }
        
        release_script_lines(p->start, p->scriptLength);
        pcb_destroy(p);
    }
}

// running shortest process first
void run_sjf() {
    int end;
    char *line;
    PCB *p;

    while (!rq_is_empty()) {
        p = rq_dequeue();
        if (p == NULL) break;

        // end is 1 past last line index
        end = p->start + p->scriptLength;
        while (p->pc < end) {
            line = get_script_line(p->pc);
            if (line != NULL && strlen(line) > 0) 
                parseInput(line);
            p->pc++;
        }

        release_script_lines(p->start, p->scriptLength);
        pcb_destroy(p);
    }
}

// SJF with aging to prevent starvation
void run_aging() {
    int end;
    char *line;
    PCB *p;
    
    while (!rq_is_empty()) {
        p = rq_dequeue_score();
        if (!p) 
        break;
        
        // end is 1 past last line index
        end = p->start + p->scriptLength;
        
        // run 1 instruction from curr process
        if (p->pc < end) {
            line = get_script_line(p->pc);
            if (line && strlen(line) > 0) 
                parseInput(line);
            p->pc++;
        }

        // check if process finished, if yes free it
        if (p->pc >= end) {
            release_script_lines(p->start, p->scriptLength);
            pcb_destroy(p);
        } 
        // else decrement scores of all waiting processes (if not 0 score)
        // and reinsert curr process into queue sorted by score
        else {
            rq_age_all();
            rq_enqueue_score(p);
        }
    }
}

// single-thread round robin
void run_rr_st(int time_quantum) {
    int end, lines_executed;
    char *line;
    PCB *p;

    while (!rq_is_empty()) {
        p = rq_dequeue();
        if (p == NULL) 
            break;

        // end is one past last line index
        end = p->start + p->scriptLength;
        lines_executed = 0;

        // run up to time_quantum lines of this process
        while (p->pc < end && lines_executed < time_quantum) {
            line = get_script_line(p->pc);
            if (line != NULL && strlen(line) > 0) parseInput(line);
            p->pc++;
            lines_executed++;
        }

        // process finishes, free it
        if (p->pc >= end) {
            release_script_lines(p->start, p->scriptLength);
            pcb_destroy(p);
        } 
        // quantum expired, so requeue for next turn
        else {
            rq_enqueue(p);
        }
    }
}

// MULTITHREADED WORKER
// worker thread loop -> dequeues and executes processes in RR slices until stopped
void *worker_loop(void *unused_arg) {
    int end, lines_executed;
    char* line;
    PCB *p;
    (void)unused_arg; // required bc pthread signature
    while (1) {
        // wait for work
        pthread_mutex_lock(&rq_mutex);

        // sleep until there's something in queue / prompted to stop
        while ((!mt_can_run || rq_is_empty()) && !mt_stop_requested) {
            pthread_cond_wait(&rq_has_work, &rq_mutex);
        }

        // check if prompted to stop
        if (mt_stop_requested && rq_is_empty()) {
            pthread_mutex_unlock(&rq_mutex);
            break;
        }

        // dequeue process
        p = rq_dequeue();
        active_workers++;
        pthread_mutex_unlock(&rq_mutex);
        
        if (!p) {
            pthread_mutex_lock(&rq_mutex);
            active_workers--;
            if (rq_is_empty() && active_workers == 0)
                pthread_cond_signal(&rq_drained);
            pthread_mutex_unlock(&rq_mutex);
            continue;
        }
        
        // end is 1 past last line index
        end = p->start + p->scriptLength;
        lines_executed = 0;

        // run up to rr_quantum lines / script end
        while (p->pc < end && lines_executed < rr_quantum) {
            line = get_script_line(p->pc);
            if (line && strlen(line) > 0) {
                // lock around interpreter bc not thread-safe
                pthread_mutex_lock(&interpreter_mutex);
                parseInput(line);
                fflush(stdout);
                pthread_mutex_unlock(&interpreter_mutex);
            }
            p->pc++;
            lines_executed++;
        }

        pthread_mutex_lock(&rq_mutex);
        active_workers--;                 // protected

        // check if process finished (free lines)
        
        if (p->pc >= end) {
            pthread_mutex_unlock(&rq_mutex); 
            release_script_lines(p->start, p->scriptLength);
            pcb_destroy(p);
            pthread_mutex_lock(&rq_mutex);
        } else {
            rq_enqueue(p);               
            pthread_cond_signal(&rq_has_work);
        }

        if (rq_is_empty() && active_workers == 0)
            pthread_cond_signal(&rq_drained);

        pthread_mutex_unlock(&rq_mutex);
        usleep(50); // so more interleaved and certain threads stop dominating
    }

    return NULL;
}

// THREAD LIFECYCLE BUSINESS
// spawns worker threads the first time it's called
void mt_start_threads_if_needed(void) {
    if (mt_threads_started)
        return;

    mt_threads_started = 1;
    mt_stop_requested = 0;

    // start each worker thread 
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&worker_threads[i], NULL, worker_loop, NULL);
    }
}

// signals all worker threads that new work might be available in RQ
void mt_notify_work(void) {
    if (!mt_threads_started) return;

    pthread_mutex_lock(&rq_mutex);
    pthread_cond_broadcast(&rq_has_work);
    pthread_mutex_unlock(&rq_mutex);
}

// pauses all worker threads by preventing them from dequeuing new processes
void scheduler_mt_pause_workers(void) {
    pthread_mutex_lock(&rq_mutex);
    mt_can_run = 0;
    pthread_mutex_unlock(&rq_mutex);
}

// resumes all paused worker threads and signals them to start processing
void scheduler_mt_resume_workers(void) {
    pthread_mutex_lock(&rq_mutex);
    mt_can_run = 1;
    pthread_cond_broadcast(&rq_has_work);
    pthread_mutex_unlock(&rq_mutex);
}

// tells all worker threads to stop (waits for them to finish then reset scheduler state)
void scheduler_mt_stop_and_join(void) {
    if (!mt_threads_started)
        return;

    // wait until queued work finishes
    pthread_mutex_lock(&rq_mutex);
    while (!rq_is_empty() || active_workers > 0) {
        pthread_cond_wait(&rq_drained, &rq_mutex);
    }

    // signal stop
    mt_stop_requested = 1;
    pthread_cond_broadcast(&rq_has_work);
    pthread_mutex_unlock(&rq_mutex);

    // wait for threads to finish
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    // reset scheduler state
    mt_threads_started = 0;
    mt_stop_requested = 0;
    scheduler_active = 0;
    scheduler_policy = INVALID;
}

// round-robin -> can run multithreaded & single-threaded round robin
void run_rr(int time_quantum) {
    // check if MT mode
    if (mt_enabled) {
        rr_quantum = time_quantum;
        
        // wake all workers
        pthread_mutex_lock(&rq_mutex);
        pthread_cond_broadcast(&rq_has_work);
        pthread_mutex_unlock(&rq_mutex);

        return;
    }
    // single thread mode
    run_rr_st(time_quantum);
}

// THE SCHEDULER
// based on policy use correct scheduling algo.
void run_scheduler(Policy policy) {
    scheduler_active = 1;
    scheduler_policy = policy;

    switch (policy) {
        case FCFS:
            run_fcfs();
            break;

        case SJF:
            run_sjf();
            break;

        case RR:
            run_rr(2);
            break;

        case RR30:
            run_rr(30);
            break;

        case AGING:
            run_aging();
            break;

        default:
            break;
    }

    if (!mt_enabled) {
        scheduler_active = 0;
        scheduler_policy = INVALID;
    }
}