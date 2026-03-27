#ifndef PCB_H
#define PCB_H

#include <stdlib.h>

struct ScriptInfo;
typedef struct ScriptInfo ScriptInfo;

typedef struct PCB {
    int pid;
    int scriptLength; // # of lines 
    int pc; // index of next instruction (starts at start)
    int score; // job length of score for AGING scheduler
    ScriptInfo *script; // shared loaded code metadata
    struct PCB *next; // link for ready queue
} PCB;

PCB *pcb_create(ScriptInfo *script);
void pcb_destroy(PCB *p);


#endif