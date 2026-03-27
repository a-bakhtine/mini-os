#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

// size constant declarations
#ifndef VAR_STORE_SIZE
#define VAR_STORE_SIZE 1000
#endif
#define MEM_SIZE VAR_STORE_SIZE // for var memory

#define MAX_LINE_LEN 100 // for script 

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 18
#endif
#define FRAME_SIZE 3
#define MAX_SCRIPTS 200

struct PCB;
typedef struct PCB PCB;

typedef struct ScriptInfo {
    char *name;
    int scriptLength; // total # lines in script
    int numPages;
    int *pageTable; // pageTable[i] = frame #
    int refCount; // # of PCBs curr referencing this script
    char **lines; // store copy of whole script
} ScriptInfo;

// shell memory
void mem_init();
void mem_set_value(char *var, char *value);
char *mem_get_value(char *var);

// paged script store
ScriptInfo *load_script(char *script_name);
ScriptInfo *load_script_from_FILE(FILE *file, const char *script_name);
void release_script(ScriptInfo *script);
char *get_pcb_script_line(PCB *p);

// paging helpers
int get_last_page_fault();
void clear_page_fault_flag();

#endif