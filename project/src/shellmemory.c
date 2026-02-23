#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

// to store lines of a script (memory)
static char *scriptStore[MAX_SCRIPT_LINES];
static int scriptStoreTop = 0;

// Helper functions
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

// load script into the global array
int load_script_lines(FILE *file, int *start, int *scriptLength) {

    if (!file || !start || !scriptLength)
        return 1;

    *start = scriptStoreTop;
    *scriptLength = 0;

    char line[MAX_LINE_LEN + 2];

    while (fgets(line, sizeof(line), file)) {
        newline_trim(line);

        if (scriptStoreTop >= MAX_SCRIPT_LINES) {
            return 1;   // out of space
        }

        scriptStore[scriptStoreTop] = strdup(line);
        if (!scriptStore[scriptStoreTop])
            return 1;

        scriptStoreTop++;
        (*scriptLength)++;
    }

    return 0;
}

// getter function
char *get_script_line(int i) {
    if (i < 0 || i >= scriptStoreTop) return NULL;
    return scriptStore[i];
}

// release function
void release_script_lines(int start, int scriptLength) {

    int end = start + scriptLength;

    for (int i = start; i < end; i++) {
        free(scriptStore[i]);
        scriptStore[i] = NULL;
    }

    // simple rollback allocator (perfect for source stage)
    if (end == scriptStoreTop) {
        scriptStoreTop = start;
    }
}