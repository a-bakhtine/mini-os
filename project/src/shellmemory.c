#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"
#include "pcb.h"

// GENERAL HELPERS
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

// SHELL VAR STORE
struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

// FRAME STORE
char *frameStore[FRAME_STORE_SIZE]; // one slot / line in every frame
int frameUsed[FRAME_STORE_SIZE / FRAME_SIZE]; // 1 = frame occupied, 0 = else
ScriptInfo *loadedScripts[MAX_SCRIPTS]; // all live ScriptInfos
int loadedScriptCount = 0; // how many scripts are loaded

ScriptInfo *frameOwner[FRAME_STORE_SIZE / FRAME_SIZE]; // what script owns the frame
int framePage[FRAME_STORE_SIZE / FRAME_SIZE]; // what page of the script is loaded (-1 is none)
unsigned long long frameLastUsed[FRAME_STORE_SIZE / FRAME_SIZE]; // LRU timestamp / frame
unsigned long long lruCounter = 0; // LRU clock 

int last_page_fault = 0; // 1 when check_page_loaded cmds a fault


// update the LRU timestamp 
void touch_frame(int frame_num) {
    if (frame_num < 0 || frame_num >= (FRAME_STORE_SIZE / FRAME_SIZE))
        return;

    frameLastUsed[frame_num] = ++lruCounter;
}

// find and return the index of a free frame (marks it used)
// returns -1 if all frames occupied
int alloc_frame() {
    int num_frames = FRAME_STORE_SIZE / FRAME_SIZE;

    for (int i = 0; i < num_frames; i++) {
        if (!frameUsed[i]) {
            frameUsed[i] = 1;
            frameLastUsed[i] = ++lruCounter;
            return i;
        }
    }
    return -1;
}

// free the heap strings stored in frame frame_num and NULL the slots
// note it does not mark the frame as unused (call site handles that)
void clear_frame_contents(int frame_num) {
    int start = frame_num * FRAME_SIZE;

    if (frame_num < 0 || frame_num >= (FRAME_STORE_SIZE / FRAME_SIZE))
        return;

    for (int i = 0; i < FRAME_SIZE; i++) {
        free(frameStore[start + i]);
        frameStore[start + i] = NULL;
    }
}

// SCRIPT HELPERS

// searches the curr loaded scripts for a script w/ provided name
// returns the ScriptInfo ptr or NULL if not found
ScriptInfo *find_loaded_script(const char *script_name) {
    for (int i = 0; i < loadedScriptCount; i++) {
        if (loadedScripts[i] && strcmp(loadedScripts[i]->name, script_name) == 0)
            return loadedScripts[i];
    }
    return NULL;
}

// PAGING

// load page_num of script into frame_num, pad missing lines w ""
// updates frame metadata and script's page table entry
void load_page_into_frame(ScriptInfo *script, int page_num, int frame_num) {
    int frame_start = frame_num * FRAME_SIZE;

    clear_frame_contents(frame_num); // drop what was here before

    for (int offset = 0; offset < FRAME_SIZE; offset++) {
        int line_index = page_num * FRAME_SIZE + offset;

        if (line_index < script->scriptLength)
            frameStore[frame_start + offset] = strdup(script->lines[line_index]);
        else
            frameStore[frame_start + offset] = strdup(""); // padding for partial last page
    }

    frameUsed[frame_num] = 1;
    frameOwner[frame_num] = script;
    framePage[frame_num] = page_num;
    frameLastUsed[frame_num] = ++lruCounter;
    script->pageTable[page_num] = frame_num; 
}

// evict the least-recently-used frame to make room for a new page
// prints victim's contents, invalidates the evicted page in its
// owner's page table, returns the freed frame index / -1 if no frame
int evict_lru_frame() {
    int num_frames = FRAME_STORE_SIZE / FRAME_SIZE;
    int victim = -1;
    unsigned long oldest = 0;
    int start;
    ScriptInfo *owner;
    int victim_page;

    // find frame with the smallest LRU timestamp (means oldest)
    for (int i = 0; i < num_frames; i++) {
        if (!frameUsed[i])
            continue;

        if (victim == -1 || frameLastUsed[i] < oldest) {
            victim = i;
            oldest = frameLastUsed[i];
        }
    }

    if (victim < 0)
        return -1;

    start = victim * FRAME_SIZE;
    owner = frameOwner[victim];
    victim_page = framePage[victim];

    // print victim page contents before eviction
    printf("Page fault! Victim page contents:\n\n");
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (frameStore[start + i] != NULL)
            printf("%s\n", frameStore[start + i]);
        else
            printf("\n");
    }
    printf("\nEnd of victim page contents.\n");


    // mark page as no longer in use in owner's page table
    if (owner != NULL && victim_page >= 0 && victim_page < owner->numPages) {
        owner->pageTable[victim_page] = -1;
    }

    // clear frame and metadata
    clear_frame_contents(victim);
    frameOwner[victim] = NULL;
    framePage[victim] = -1;
    frameLastUsed[victim] = 0;
    frameUsed[victim] = 0;

    return victim;
}

// check page_num of script exists in some frame
// if already loaded, return, else load then return
void check_page_loaded(ScriptInfo *script, int page_num) {
    int frame_num;

    if (!script || page_num < 0 || page_num >= script->numPages)
        return;

    // check if exists in memory already
    if (script->pageTable[page_num] != -1)
        return;

    last_page_fault = 1;

    // try to alloc to free frame
    frame_num = alloc_frame();
    if (frame_num >= 0) {
        printf("Page fault!\n");
        load_page_into_frame(script, page_num, frame_num);
        return;
    }

    // no free frames so evict LRU and load into reclaimed frame
    frame_num = evict_lru_frame();
    if (frame_num >= 0)
        load_page_into_frame(script, page_num, frame_num);
}

// MEMORY FUNCTIONS

// init var store and frame store to empty state
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
        frameLastUsed[i] = 0;
    }

    lruCounter = 0;
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

// SCRIPT LOADING

// read a script from a FILE, build ScriptInfo obj, and 
// load the first 2 pages into frames
ScriptInfo *load_script_from_FILE(FILE *file, const char *script_name) {
    char line[MAX_LINE_LEN + 2];
    char **lines = NULL;
    int line_capacity = 0;
    int line_count = 0;
    ScriptInfo *script = NULL;

    if (!file || !script_name)
        return NULL;

    // check if duplicate (script_name already loaded)
    if (find_loaded_script(script_name) != NULL)
        return NULL;

    // read all lines to a dynm arr
    while (fgets(line, sizeof(line), file)) {
        char *copy;

        newline_trim(line);
        
        // grow the lines buffer if full (doubles each time)
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

    // build the ScriptInfo obj
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
        // alloc page table and init all entries to -1
        script->pageTable = malloc(script->numPages * sizeof(int));
        if (!script->pageTable)
            goto fail;

        for (int i = 0; i < script->numPages; i++) {
            script->pageTable[i] = -1;
        }

        // load page 0
        {
            int first = alloc_frame();
            if (first < 0)
                goto fail;
            load_page_into_frame(script, 0, first);
        }

        // load page 1 if exists
        if (script->numPages > 1) {
            int second = alloc_frame();
            if (second >= 0)
                load_page_into_frame(script, 1, second);
        }
    }

    if (loadedScriptCount >= MAX_SCRIPTS)
        goto fail;

    // load the script
    loadedScripts[loadedScriptCount++] = script;
    return script;

fail:
    // rollback frames that have been alloc to this script
    if (script) {
        if (script->pageTable) {
            for (int i = 0; i < script->numPages; i++) {
                if (script->pageTable[i] >= 0) {
                    int frame_num = script->pageTable[i];
                    clear_frame_contents(frame_num);
                    frameUsed[frame_num] = 0;
                    frameOwner[frame_num] = NULL;
                    framePage[frame_num] = -1;
                    frameLastUsed[frame_num] = 0;
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

// load a script by file-system path
ScriptInfo *load_script(char *script_name) {
    FILE *file;
    ScriptInfo *script;

    if (!script_name)
        return NULL;

    // check if already loaded
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

// decr the refcount of script
void release_script(ScriptInfo *script) {
    if (!script)
        return;

    if (script->refCount > 0)
        script->refCount--;

}

// PCB HELPER

// return line of script text that p's pc currently points at
char *get_pcb_script_line(PCB *p) {
    int page_num;
    int offset;
    int frame_num;
    int line_index;

    last_page_fault = 0; // reset before each access

    if (!p || !p->script)
        return NULL;
    if (p->pc < 0 || p->pc >= p->scriptLength)
        return NULL;

    page_num = p->pc / FRAME_SIZE; // page that holds line
    offset = p->pc % FRAME_SIZE; // line's pos in the page

    if (page_num < 0 || page_num >= p->script->numPages)
        return NULL;

    // check if page not loaded 
    if (p->script->pageTable[page_num] == -1) {
        check_page_loaded(p->script, page_num);
        return NULL;
    }

    frame_num = p->script->pageTable[page_num];
    line_index = frame_num * FRAME_SIZE + offset;

    if (line_index < 0 || line_index >= FRAME_STORE_SIZE)
        return NULL;

    // update LRU so the frame isn't evicted prematurely
    touch_frame(frame_num);
    return frameStore[line_index];
}

// PAGE-FAULT HELPERS

// get bool if last get_pcb_script_line call triggered a page fault
int get_last_page_fault() {
    return last_page_fault;
}

// reset page-fault flag
void clear_page_fault_flag() {
    last_page_fault = 0;
}
