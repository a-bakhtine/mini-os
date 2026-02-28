#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#include <stdio.h>

#define MEM_SIZE 1000 // for var memory
#define MAX_SCRIPT_LINES 1000 // for script mem
#define MAX_LINE_LEN 100 // for script mem

// shell memory
void mem_init();
void mem_set_value(char *var, char *value);
char *mem_get_value(char *var);

// script store
int load_script_lines(FILE *file, int *start, int *scriptLength);
char *get_script_line(int index);
void release_script_lines(int start, int count);

#endif