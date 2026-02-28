#include "pcb.h"

int next_pid = 1; // pid counter

// allocate and init new PCB for a script loaded at [start, start+scriptLength] 
PCB *pcb_create(int start, int scriptLength) {
    PCB *p = (PCB *)malloc(sizeof(PCB));
    if (p == NULL) 
        return NULL;

    p->pid = next_pid++;
    p->start = start;
    p->scriptLength = scriptLength;
    p->pc = start;
    p->score = p->scriptLength;

    p->next = NULL;

    return p;
}

// free PCB
void pcb_destroy(PCB *p) {
    if (p)
        free(p);
}