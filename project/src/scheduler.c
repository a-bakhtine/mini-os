#include <string.h>
#include "scheduler.h"
#include "readyqueue.h"
#include "shellmemory.h"
#include "pcb.h"
#include "interpreter.h"

// policy string -> enum
Policy parse_policy(const char *s) {
    if (strcmp(s, "FCFS") == 0) return FCFS;
    if (strcmp(s, "SJF")  == 0) return SJF;
    if (strcmp(s, "RR")   == 0) return RR;
    if (strcmp(s, "AGING")== 0) return AGING;
    return INVALID;
}

// run all enqueued process to completion in arrival order
void run_fcfs() {
    while (!rq_is_empty()) {

        PCB *p = rq_dequeue();
        if (p == NULL)
            break;
            
        // one past last line index
        int end = p->start + p->scriptLength;

        while (p->pc < end) {
            char *line = get_script_line(p->pc);
            
            // skip null lines
            if (line != NULL && strlen(line) > 0) {
                parseInput(line);
            }
            p->pc++;
        }
        
        pcb_destroy(p);
    }
}

// based on policy use correct scheduling algo.
void run_scheduler(Policy policy) {
    switch (policy) {
        case FCFS:
            run_fcfs();
            break;
        default:
            break;
    }
}
