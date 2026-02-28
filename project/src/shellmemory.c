#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

// HELPERS
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i]) matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else return 0;
}

// remove newline
void newline_trim(char *s) {
    if (!s) 
        return;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\n' || s[i] == '\r') {
            s[i] = '\0';
            return;
        }
    }
}


// SHELL MEMORY
struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

// shell memory functions
void mem_init(){
    int i;
    for (i = 0; i < MEM_SIZE; i++){		
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, "none") == 0){
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        } 
    }

    return;
}

// get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++){
        if (strcmp(shellmemory[i].var, var_in) == 0){
            return strdup(shellmemory[i].value);
        } 
    }
    return "Variable does not exist";
}


// SCRIPT STORE

// to store lines of a script (memory)
char *scriptStore[MAX_SCRIPT_LINES];
int scriptStoreTop = 0;

// reads all lines from a file into global script store, sets *start and *scriptLength 
int load_script_lines(FILE *file, int *start, int *scriptLength) {
    if (!file || !start || !scriptLength)
        return 1;

    *start = scriptStoreTop;
    *scriptLength = 0;

    char line[MAX_LINE_LEN + 2]; // +1 for '\n', +1 for '\0'
    while (fgets(line, sizeof(line), file)) {
        newline_trim(line);

        // check if out of space
        if (scriptStoreTop >= MAX_SCRIPT_LINES) {
            return 1;
        }

        scriptStore[scriptStoreTop] = strdup(line);
        // check if out of memory
        if (!scriptStore[scriptStoreTop])
            return 1;

        scriptStoreTop++;
        (*scriptLength)++;
    }

    return 0;
}

// returns the script line @ index i / NULL (out of bounds)
char *get_script_line(int i) {
    if (i < 0 || i >= scriptStoreTop) return NULL;
    return scriptStore[i];
}

// frees script lines in range [start, start+scriptLength]
void release_script_lines(int start, int scriptLength) {
    int end = start + scriptLength;

    for (int i = start; i < end; i++) {
        free(scriptStore[i]);
        scriptStore[i] = NULL;
    }

    // if range @ top of store, rollback allocator ptr
    if (end == scriptStoreTop) {
        scriptStoreTop = start;
    }
}