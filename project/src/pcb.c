#include "pcb.h"
#include "shellmemory.h"

int next_pid = 1; // pid counter

// allocate and init new PCB for a script loaded at [start, start+scriptLength] 
PCB *pcb_create(ScriptInfo *script) {
    PCB *p = (PCB *)malloc(sizeof(PCB));
    if (p == NULL || script == NULL) 
        return NULL;

    p->pid = next_pid++;
    p->start = 0;
    p->scriptLength = script->scriptLength;
    p->pc = 0;
    p->score = p->scriptLength;
    p->script = script;
    p->next = NULL;

    return p;
}

// free PCB
void pcb_destroy(PCB *p) {
    if (p)
        if (p->script)
            release_script(p->script);
        free(p);
}