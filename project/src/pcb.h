#ifndef PCB_H
#define PCB_H

#include <stdlib.h>

typedef struct PCB {
    int pid;

    int start; // start index of script_store
    int scriptLength; // # of lines 
    int pc; // index of next instruction (starts at start)
    int score; // job lenth of score for AGING scheduler
    struct PCB *next; // link for ready queue
} PCB;

PCB *pcb_create(int start, int scriptLength);
void pcb_destroy(PCB *p);


#endif