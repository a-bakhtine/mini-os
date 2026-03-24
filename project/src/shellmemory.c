#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"
#include "pcb.h"

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

// FRAME STORE
char *frameStore[FRAME_STORE_SIZE];
int frameUsed[FRAME_STORE_SIZE / FRAME_SIZE];
ScriptInfo *loadedScripts[MAX_SCRIPTS];
int loadedScriptCount = 0;


ScriptInfo *frameOwner[FRAME_STORE_SIZE / FRAME_SIZE];
int framePage[FRAME_STORE_SIZE / FRAME_SIZE];

int last_page_fault = 0;

int alloc_frame() {
    int num_frames = FRAME_STORE_SIZE / FRAME_SIZE;

    for (int i = 0; i < num_frames; i++) {
        if (!frameUsed[i]) {
            frameUsed[i] = 1;
            return i;
        }
    }
    return -1;
}

void clear_frame_contents(int frame_num) {
    int start = frame_num * FRAME_SIZE;

    if (frame_num < 0 || frame_num >= (FRAME_STORE_SIZE / FRAME_SIZE))
        return;

    for (int i = 0; i < FRAME_SIZE; i++) {
        free(frameStore[start + i]);
        frameStore[start + i] = NULL;
    }
}

ScriptInfo *find_loaded_script(const char *script_name) {
    for (int i = 0; i < loadedScriptCount; i++) {
        if (loadedScripts[i] && strcmp(loadedScripts[i]->name, script_name) == 0)
            return loadedScripts[i];
    }
    return NULL;
}

void unregister_script(ScriptInfo *script) {
    for (int i = 0; i < loadedScriptCount; i++) {
        if (loadedScripts[i] == script) {
            for (int j = i; j < loadedScriptCount - 1; j++) {
                loadedScripts[j] = loadedScripts[j + 1];
            }
            loadedScripts[loadedScriptCount - 1] = NULL;
            loadedScriptCount--;
            return;
        }
    }
}


static void load_page_into_frame(ScriptInfo *script, int page_num, int frame_num) {
    int frame_start = frame_num * FRAME_SIZE;

    clear_frame_contents(frame_num);

    for (int offset = 0; offset < FRAME_SIZE; offset++) {
        int line_index = page_num * FRAME_SIZE + offset;

        if (line_index < script->scriptLength)
            frameStore[frame_start + offset] = strdup(script->lines[line_index]);
        else
            frameStore[frame_start + offset] = strdup("");
    }

    frameUsed[frame_num] = 1;
    frameOwner[frame_num] = script;
    framePage[frame_num] = page_num;
    script->pageTable[page_num] = frame_num;
}

int evict_random_frame() {
    int num_frames = FRAME_STORE_SIZE / FRAME_SIZE;
    int victim = rand() % num_frames;
    int start = victim * FRAME_SIZE;
    ScriptInfo *owner = frameOwner[victim];
    int victim_page = framePage[victim];

    printf("Page fault!\n");
    printf("Victim page contents:\n");
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (frameStore[start + i] != NULL)
            printf("%s\n", frameStore[start + i]);
        else
            printf("\n");
    }
    printf("End of victim page contents.\n");

    if (owner != NULL && victim_page >= 0 && victim_page < owner->numPages) {
        owner->pageTable[victim_page] = -1;
    }

    clear_frame_contents(victim);
    frameOwner[victim] = NULL;
    framePage[victim] = -1;
    frameUsed[victim] = 0;

    return victim;
}

void ensure_page_loaded(ScriptInfo *script, int page_num) {
    int frame_num;

    if (!script || page_num < 0 || page_num >= script->numPages)
        return;

    if (script->pageTable[page_num] != -1)
        return;

    last_page_fault = 1;

    frame_num = alloc_frame();
    if (frame_num >= 0) {
        printf("Page fault!\n");
        load_page_into_frame(script, page_num, frame_num);
        return;
    }

    frame_num = evict_random_frame();
    load_page_into_frame(script, page_num, frame_num);
}

// memory functions
void mem_init() {
    int i;
    int num_frames = FRAME_STORE_SIZE / FRAME_SIZE;

    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }

    for (i = 0; i < FRAME_STORE_SIZE; i++) {
        frameStore[i] = NULL;
    }

    for (i = 0; i < num_frames; i++) {
        frameUsed[i] = 0;
        frameOwner[i] = NULL;
        framePage[i] = -1;
    }

    srand((unsigned int) time(NULL));
}

// set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
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

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return "Variable does not exist";
}

ScriptInfo *load_script_from_FILE(FILE *file, const char *script_name) {
    char line[MAX_LINE_LEN + 2];
    char **lines = NULL;
    int line_capacity = 0;
    int line_count = 0;
    ScriptInfo *script = NULL;

    if (!file || !script_name)
        return NULL;

    if (find_loaded_script(script_name) != NULL)
        return NULL;

    while (fgets(line, sizeof(line), file)) {
        char *copy;

        newline_trim(line);

        if (line_count == line_capacity) {
            int new_capacity = (line_capacity == 0) ? 8 : line_capacity * 2;
            char **new_lines = realloc(lines, new_capacity * sizeof(char *));
            if (!new_lines)
                goto fail;
            lines = new_lines;
            line_capacity = new_capacity;
        }

        copy = strdup(line);
        if (!copy)
            goto fail;
        lines[line_count++] = copy;
    }

    script = malloc(sizeof(ScriptInfo));
    if (!script)
        goto fail;

    script->name = strdup(script_name);
    if (!script->name)
        goto fail;

    script->scriptLength = line_count;
    script->numPages = (line_count + FRAME_SIZE - 1) / FRAME_SIZE;
    script->refCount = 1;
    script->lines = lines;
    script->pageTable = NULL;

    if (script->numPages > 0) {
        script->pageTable = malloc(script->numPages * sizeof(int));
        if (!script->pageTable)
            goto fail;

        for (int i = 0; i < script->numPages; i++) {
            script->pageTable[i] = -1;
        }

        {
            int first = alloc_frame();
            if (first < 0)
                goto fail;
            load_page_into_frame(script, 0, first);
        }

        if (script->numPages > 1) {
            int second = alloc_frame();
            if (second < 0)
                goto fail;
            load_page_into_frame(script, 1, second);
        }
    }

    if (loadedScriptCount >= MAX_SCRIPTS)
        goto fail;

    loadedScripts[loadedScriptCount++] = script;
    return script;

fail:
    if (script) {
        if (script->pageTable) {
            for (int i = 0; i < script->numPages; i++) {
                if (script->pageTable[i] >= 0) {
                    int frame_num = script->pageTable[i];
                    clear_frame_contents(frame_num);
                    frameUsed[frame_num] = 0;
                    frameOwner[frame_num] = NULL;
                    framePage[frame_num] = -1;
                }
            }
            free(script->pageTable);
        }
        free(script->name);
        free(script);
    }
    if (lines) {
        for (int i = 0; i < line_count; i++)
            free(lines[i]);
        free(lines);
    }
    return NULL;
}

ScriptInfo *load_script(char *script_name) {
    FILE *file;
    ScriptInfo *script;

    if (!script_name)
        return NULL;

    script = find_loaded_script(script_name);
    if (script) {
        script->refCount++;
        return script;
    }

    file = fopen(script_name, "rt");
    if (!file)
        return NULL;

    script = load_script_from_FILE(file, script_name);
    fclose(file);
    return script;
}

void release_script(ScriptInfo *script) {
    if (!script)
        return;

    script->refCount--;
    if (script->refCount > 0)
        return;

    unregister_script(script);

    if (script->lines) {
        for (int i = 0; i < script->scriptLength; i++) {
            free(script->lines[i]);
        }
        free(script->lines);
    }

    free(script->pageTable);
    free(script->name);
    free(script);
}

char *get_pcb_script_line(PCB *p) {
    int page_num;
    int offset;
    int frame_num;
    int line_index;

    last_page_fault = 0;

    if (!p || !p->script)
        return NULL;
    if (p->pc < 0 || p->pc >= p->scriptLength)
        return NULL;

    page_num = p->pc / FRAME_SIZE;
    offset = p->pc % FRAME_SIZE;

    if (page_num < 0 || page_num >= p->script->numPages)
        return NULL;

    if (p->script->pageTable[page_num] == -1) {
        ensure_page_loaded(p->script, page_num);
        return NULL;
    }

    frame_num = p->script->pageTable[page_num];
    line_index = frame_num * FRAME_SIZE + offset;

    if (line_index < 0 || line_index >= FRAME_STORE_SIZE)
        return NULL;

    return frameStore[line_index];
}

int page_fault_happened() {
    return last_page_fault;
}

void clear_page_fault_flag() {
    last_page_fault = 0;
}
