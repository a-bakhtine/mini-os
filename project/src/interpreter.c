#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "shellmemory.h"
#include "shell.h"
#include "pcb.h"
#include "readyqueue.h"
#include "scheduler.h"

int MAX_ARGS_SIZE = 5;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommand_file_DNE() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommand_specific(char *value) {
    printf("Bad command: %s\n", value);
    return 1;
}

int is_alphanumeric(const char *s) {
    int i;

    if (s == NULL || s[0] == '\0') 
        return 0;
    for (i = 0; s[i] != '\0'; i++) {
        if (!((s[i] >= '0' && s[i] <= '9') ||
        (s[i] >= 'A' && s[i] <= 'Z') ||
        (s[i] >= 'a' && s[i] <= 'z'))) 
            return 0; // not alphanumeric
    }
    return 1; // is alphanum
}


int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int echo(char *token);
int my_ls();
int my_mkdir(char *token);
int my_touch(char *value);
int my_cd(char *value);
int run(char *command_args[], int args_size);
int exec(char *command_args[], int args_size);
int badcommand_file_DNE();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);
    } else if (strcmp(command_args[0], "echo") == 0 ) {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand();
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);
    
    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        return run(command_args, args_size);
    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3 || args_size > 5)
            return badcommand();
        return exec(command_args, args_size);
    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n \
echo STRING            Display STRING or value of $VAR\n \
my_ls                  Displays all files present in the current directory\n \
my_mkdir STRING        Creates new directory with name STRING in the current directory\n \
my_touch STRING        Creates a new empty file in the current directory\n \
my_cd STRING           Changes the current directory to directory STRING\n \
run COMMAND            Run command to perform testcases\n \
exec p1 p2 p3 POLICY   Runs multiple processes (1-3) concurrently with given POLICY";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}


int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int source(char *script) {
    int start, scriptLength, load_status;
    PCB *proc;
    FILE *p = fopen(script, "rt");
    
    if (p == NULL)
        return badcommand_file_DNE();

    load_status = load_script_lines(p, &start, &scriptLength);
    fclose(p);
    if (load_status != 0) 
        return load_status;

    proc = pcb_create(start, scriptLength);
    if (proc == NULL) { // error
        release_script_lines(start, scriptLength);
        return 1;
    }

    // init and setup readyqueue
    rq_enqueue(proc);

    // FCFS scheduler
    run_scheduler(FCFS);

    // remove script code from mem
    release_script_lines(start, scriptLength);

    return 0;
}

// prints a token followed by a newline
int echo(char *token) {
    // check shell memory for a var with the following name after $
    if (token[0] == '$') {
        char *var = token + 1; // skip "$"
        token = mem_get_value(var); 
        
        // no variable following variable prompt
        if (strcmp(token, "Variable does not exist") == 0) {
            token = ""; // echo prints blank line for missing var
        }         
    }

    printf("%s\n", token);
    return 0;
}

// lists directory entries in curr directory in alphabetical order
int my_ls() {
    struct dirent **entry;
    int i, scan;
    const char* dir_path = ".";

    // allocates array of dirent* and each dirent* & sort alphabetically
    scan = scandir(dir_path, &entry, NULL, alphasort); 
    // error
    if (scan < 0) 
        return 1;
    
    // print each filename on its own line, then free the allocated slot
    for (i = 0; i < scan; i++) {
        printf("%s\n", entry[i]->d_name);
        free(entry[i]);
    }
    free(entry);

    return 0;
}

// creates directory with given name / var value
int my_mkdir(char *token) {
    char *dirname = token;
    
    // check shell memory for dir name from variable
    if (dirname[0] == '$') {
        char *var = dirname + 1;
        dirname = mem_get_value(var);

        // var DNE
         if (dirname == NULL || strcmp(dirname, "Variable does not exist") == 0) {
            return badcommand_specific("my_mkdir");
        }      
    } 

    if (!is_alphanumeric(dirname)) {
        return badcommand_specific("my_mkdir");
    }

    // full perms for every group and check for error
    if (mkdir(dirname, 0777) != 0) {
        return 1;
    }

    return 0;
}

// creates file if DNE / updates "last modified" time if exists
int my_touch(char *value) {
    FILE *file;

    if (!is_alphanumeric(value))
        return badcommand_specific("my_touch");

    // open file to create it
    file = fopen(value, "a");
    // check error
    if (file == NULL) 
        return badcommand_specific("my_touch"); 

    fclose(file);
    return 0;
}

// changes current working directory
int my_cd(char *value) {
    // must be alphanumeric directory and check if error in entering directory
    if ((!is_alphanumeric(value) && strcmp(".", value) != 0 && 
    strcmp("..", value) != 0) || chdir(value) != 0) 
        return badcommand_specific("my_cd");
       
    return 0;
}

// executes external program in child process
int run(char *command_args[], int args_size) {
    char *argv[args_size];  // max possible entries 
    int i, status;
    pid_t pid;
    
    // argv[0] = program name
    // build argv for execvp -> skip run cmd and null-terminator
    for (i = 1; i < args_size; i++) {
        argv[i - 1] = command_args[i];
    }
    argv[args_size - 1] = NULL;

    pid = fork();

    // fork error (can't make child process)
    if (pid < 0)
        return badcommand_specific("run");

    if (pid == 0) {
        // child: replace process w/ desired program
        if (execvp(argv[0], argv) < 0) // error catch
             _exit(1); 

        // execvp error
        exit(1);
    } else {
        // parent: waits for child to finish
        waitpid(pid, &status, 0);
        return 0;
    }

}

void cleanup_loaded_scripts(int frame_starts[], int frame_lens[], PCB *pcbs[], int scripts_loaded, int total) {
    rq_init();
    // release script mem
    for (int i = scripts_loaded - 1; i >= 0; i--)
        release_script_lines(frame_starts[i], frame_lens[i]);
    // destroy any created PCBs 
    for (int i = 0; i < total; i++)
        if (pcbs[i]) pcb_destroy(pcbs[i]);
}

int exec(char *command_args[], int args_size) {
    // parse policy (last argument)
    Policy policy = parse_policy(command_args[args_size - 1]);
    if (policy == INVALID)
        return 1;

    rq_init();
    // parse script names (between 1st arg and last)
    int script_count = args_size - 2;
    char *script_names[3];
    for (int i = 0; i < script_count; i++)
        script_names[i] = command_args[1 + i];

    // check for duplicate script names 
    for (int i = 0; i < script_count; i++) {
        for (int j = i + 1; j < script_count; j++) {
            if (strcmp(script_names[i], script_names[j]) == 0)
                return 1;
        }
    }

    // track loaded scripts (to cleanup if fail)
    int frame_starts[3] = {0};
    int frame_lens[3] = {0};
    PCB *pcbs[3] = {NULL};
    int scripts_loaded = 0;

    // load each script into memory, create its PCB, enqueue
    for (int i = 0; i < script_count; i++) {
        FILE *script_file = fopen(script_names[i], "rt");
        
        // check if file found
        if (!script_file) {
            cleanup_loaded_scripts(frame_starts, frame_lens, pcbs, scripts_loaded, script_count);
            return 1; 
        }

        int frame_start, frame_len;
        int load_result = load_script_lines(script_file, &frame_start, &frame_len);
        fclose(script_file);

        // check if enough memory / load errors
        if (load_result != 0) {
            cleanup_loaded_scripts(frame_starts, frame_lens, pcbs, scripts_loaded, script_count);
            return 1; 
        }

        frame_starts[i] = frame_start;
        frame_lens[i] = frame_len;
        scripts_loaded++;

        pcbs[i] = pcb_create(frame_start, frame_len);
        // check if PCB allocation worked
        if (!pcbs[i]) {
            cleanup_loaded_scripts(frame_starts, frame_lens, pcbs, scripts_loaded, script_count);
            return 1; 
        }

        // enqueue
        if (policy == SJF) {
            rq_enqueue_sjf(pcbs[i]);
        } else if (policy == AGING) {
            rq_enqueue_score(pcbs[i]);
        } else {
            rq_enqueue(pcbs[i]);
        }
    }

    // all scripts loaded, pass to scheduler
    run_scheduler(policy);

    // release script memory 
    for (int i = script_count - 1; i >= 0; i--)
        release_script_lines(frame_starts[i], frame_lens[i]);

    return 0;
}