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


int scheduler_is_active();
Policy scheduler_get_policy();
Policy parse_policy(const char *s);
void run_scheduler(Policy policy);

// thread-safe rq operations 
void scheduler_rq_enqueue(PCB *p);
PCB* scheduler_rq_dequeue();
int scheduler_rq_is_empty();
void scheduler_rq_init();
void scheduler_rq_enqueue_front(PCB *p);

// MT controls
void scheduler_mt_enable(); 
int scheduler_mt_enabled();
void scheduler_mt_stop_and_join(); 
void scheduler_mt_notify_work();

// rq lock helpers
void scheduler_rq_lock();
void scheduler_rq_unlock();

#endif