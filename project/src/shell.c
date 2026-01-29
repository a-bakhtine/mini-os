#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.5 created Dec 2025\n");

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }
    
    //init shell memory
    mem_init();
    int mode = isatty(STDIN_FILENO); // 1 terminal, 0 else
    while(1) {							
        if (mode) {
            printf("%c ", prompt);
        }
        // here you should check the unistd library 
        // so that you can find a way to not display $ in the batch mode
        if (fgets(userInput, MAX_USER_INPUT-1, stdin) == NULL) {
           break; 
        }
        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

int wordEnding(char c) {
    return c == '\0' || c == '\n' || c == ' ';
}

int parseOneCommand(char inp[]) {
    char tmp[200], *words[100];                            
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;
    for (ix = 0; inp[ix] == ' ' && ix < 1000; ix++); // skip white spaces
    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // extract a word
        for (wordlen = 0; !wordEnding(inp[ix]) && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = inp[ix];                        
        }
        tmp[wordlen] = '\0';
        words[w] = strdup(tmp);
        w++;
        if (inp[ix] == '\0') break;
        ix++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;

}

int parseInput(char inp[]) {
    int lastError;
    char lineCopy[1001];
    char *cmd;
    int count;

    lastError = 0;
    
    // work on copy bc strtok modifies 
    strncpy(lineCopy, inp, 1000);
    lineCopy[1000] = '\0';

    // splits on semi colons
    cmd = strtok(lineCopy, ";");
    count = 0;

    // assume @ most 10 chained instructions
    while (cmd != NULL && count < 10) { 
        lastError = parseOneCommand(cmd);
        cmd = strtok(NULL, ";");
        count++;
    }

    return lastError;
}
