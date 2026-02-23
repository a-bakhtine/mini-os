#include <string.h>
#include "scheduler.h"
#include "readyqueue.h"
#include "shellmemory.h"
#include "pcb.h"
#include "interpreter.h"

void run_fcfs() {
    while (!rq_is_empty()) {

        PCB *p = rq_dequeue();
        if (p == NULL)
            break;
            
        // one past last line
        int end = p -> start + p -> scriptLength;

        while (p -> pc < end) {
            char *line = get_script_line(p -> pc);

            // skip null lines
            if (line != NULL && strlen(line) > 0) {
                parseInput(line);
            }
            p -> pc++;
        }

        pcb_destroy(p);
    }
}