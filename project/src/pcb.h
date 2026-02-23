#ifndef PCB_H
#define PCB_H

#include <stdlib.h>

typedef struct PCB {
    int pid;

    int start; // start index in script_store
    int scriptLength; // number of lines in this script
    int pc; // index of next instruction (starts at start)

    struct PCB *next;   // link for ready queue
} PCB;

PCB *pcb_create(int start, int scriptLength);
void pcb_destroy(PCB *p);


#endif