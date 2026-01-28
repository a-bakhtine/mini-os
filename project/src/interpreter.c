#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 3;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
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
int badcommandFileDoesNotExist();

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
my_cd STRING           Changes the current directory to directory STRING";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}


int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);     // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
}

int echo(char *token) {
    // check shell memory for a var with the following name after $
    if (token[0] == '$') {
        char *var = token + 1; // skip "$"
        token = mem_get_value(var);
        
        if (strcmp(token, "Variable does not exist") == 0) {
            token = ""; // echo prints blank line for missing var
        }         
    }

    printf("%s\n", token);
    return 0;
}

int my_ls() {
    struct dirent **entry;
    int i, scan;
    const char* dir_path = ".";

    scan = scandir(dir_path, &entry, NULL, alphasort); // sorts entries alphabetically
    if (scan < 0) {
        perror("my_ls\n");
        return 1;
    }

    for (i = 0; i < scan; i++) {
        printf("%s\n", entry[i]->d_name);
        free(entry[i]);
    }
    free(entry);

    return 0;
}

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

    // full perms for every group
    if (mkdir(dirname, 0777) != 0) {
        perror("my_mkdir failed\n");
        return 1;
    }

    return 0;
}

int my_touch(char *value) {
    FILE *file;

    if (!is_alphanumeric(value))
        return badcommand_specific("my_touch");

    file = fopen(value, "a");
    if (file == NULL) 
        return badcommand_specific("my_touch"); 

    fclose(file);
    return 0;
}

int my_cd(char *value) {
    if ((!is_alphanumeric(value) && strcmp(".", value) != 0 && 
    strcmp("..", value) != 0) || chdir(value) != 0) 
        return badcommand_specific("my_cd");
       
    return 0;
}

