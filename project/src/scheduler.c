#include <string.h>
#include "scheduler.h"
#include "readyqueue.h"
#include "shellmemory.h"
#include "pcb.h"
#include "interpreter.h"

// policy string -> enum
Policy parse_policy(const char *s) {
    if (strcmp(s, "FCFS") == 0) return FCFS;
    if (strcmp(s, "SJF") == 0) return SJF;
    if (strcmp(s, "RR") == 0) return RR;
    if (strcmp(s, "RR30") == 0) return RR30;
    if (strcmp(s, "AGING")== 0) return AGING;
    return INVALID;
}

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
        
        release_script_lines(p->start, p->scriptLength)
        pcb_destroy(p);
    }
}

// running shortest scrit first
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
            if (line != NULL && strlen(line) > 0) parseInput(line);
            p->pc++;
        }

        release_script_lines(p->start, p->scriptLength)
        pcb_destroy(p);
    }
}

void run_rr(int time_quantum) {
    int end, lines_executed;
    char *line;
    PCB *p;

    while (!rq_is_empty()) {
        p = rq_dequeue();
        if (p == NULL) break;

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
            release_script_lines(p->start, p->scriptLength)
            pcb_destroy(p);
        } 
        // quantum expired, so requeue for next turn
        else {
            rq_enqueue(p);
        }
    }
}

void run_aging() {
    int end;
    char *line;
    PCB *p;

    while (!rq_is_empty()) {
        p = rq_dequeue();
        if (!p) break;

        // end is 1 past last line index
        end = p->start + p->scriptLength;

        // run 1 instruction from curr process
        if (p->pc < end) {
            line = get_script_line(p->pc);
            if (line && strlen(line) > 0) parseInput(line);
            p->pc++;
        }

        // check if process finished, if yes free it
        if (p->pc >= end) {
            release_script_lines(p->start, p->scriptLength)
            pcb_destroy(p);
        } 
        // else decrement scores of all waiting processes (if not 0 score)
        // and reinsert curr process into queue sorted by score
        else {
            if (p->score > 0)
                rq_age_all();
            rq_enqueue_score(p);
        }
    }
}

// based on policy use correct scheduling algo.
void run_scheduler(Policy policy) {
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
}
