#ifndef SCHEDULER_H
#define SCHEDULER_H

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

#endif