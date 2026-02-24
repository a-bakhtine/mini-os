#include "pcb.h"

int next_pid = 1;

PCB *pcb_create(int start, int scriptLength) {
    PCB *p = (PCB *)malloc(sizeof(PCB));
    if (p == NULL) 
        return NULL;

    p->pid = next_pid++;
    p->start = start;
    p->scriptLength = scriptLength;
    p->pc = start;

    p->next = NULL;

    return p;
}

void pcb_destroy(PCB *p) {
    if (p)
        free(p);
}