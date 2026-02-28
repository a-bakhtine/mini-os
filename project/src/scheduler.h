#ifndef SCHEDULER_H
#define SCHEDULER_H

struct PCB;
typedef struct PCB PCB;

typedef enum {
    FCFS,
    SJF,
    RR,
    RR30,
    AGING,
    INVALID
} Policy;

// scheduler functions
int scheduler_is_active();
Policy scheduler_get_policy();
Policy parse_policy(const char *s);

//multithreading functions
void scheduler_mt_enable(); 
int scheduler_mt_enabled();
int  scheduler_mt_is_worker_thread(void);

// thread-safe RQ wrappers
void scheduler_rq_init();
void scheduler_rq_enqueue(PCB *p);
void scheduler_rq_enqueue_front(PCB *p);

// thread lifecycle
void mt_start_threads_if_needed(void);
void mt_notify_work(void);
void scheduler_mt_pause_workers(void);
void scheduler_mt_resume_workers(void);
void scheduler_mt_stop_and_join(); 

// the scheduler 
void run_scheduler(Policy policy);
#endif