#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

#define VAR_STORE_SIZE 1000
#define MEM_SIZE VAR_STORE_SIZE // for var memory

#define MAX_SCRIPT_LINES 100 // total frame-store lines
#define MAX_LINE_LEN 100 // for script 

#define FRAME_STORE_SIZE 18
#define FRAME_SIZE 3
#define MAX_SCRIPTS 200

struct PCB;
typedef struct PCB PCB;

typedef struct ScriptInfo {
    char *name;
    int scriptLength;
    int numPages;
    int *pageTable; // pageTable[i] = frame #
    int refCount;
    char **lines; // store copy of whole script
} ScriptInfo;

// shell memory
void mem_init();
void mem_set_value(char *var, char *value);
char *mem_get_value(char *var);

// paged script store
ScriptInfo *load_script(char *script_name);
ScriptInfo *load_script_from_FILE(FILE *file, const char *script_name);
void keep_script(ScriptInfo *script);
void release_script(ScriptInfo *script);
char *get_pcb_script_line(PCB *p);

// paging helpers
int page_fault_happened();
void clear_page_fault_flag();

#endif