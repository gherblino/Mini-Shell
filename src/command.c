#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "command.h"

/// Initialises the global InputLine variable
void newInput() {
    wantsToExit = 0;
    inputLine.len = 0;
    inputLine.commands = malloc(inputLine.len * sizeof(Command));
    inputLine.errorOccurred = 0;
}

/// Add a new Command c to the global InputLine variable
void addCommand(Command c, int runInBackground) {
    c.runInBackground = runInBackground;

    inputLine.commands = realloc(inputLine.commands, (++inputLine.len) * sizeof(Command));
    inputLine.commands[inputLine.len-1] = c;
}

/// Appends an empty Command c to the end of the global InputLine variable
void appendEmptyCommand(int runInBackground) {
    Command c;

    // CommandList
    c.cl.len = 1;
    c.cl.cp = malloc(inputLine.len * sizeof(CommandPart));

    // First CommandPart
    c.cl.cp[0].command = NULL;

    // Options of First CommandPart
    c.cl.cp[0].options.arr = NULL;
    c.cl.cp[0].options.len = 0;

    // IO
    c.io.input = NULL;
    c.io.output = NULL;

    // Add this new empty command using the already existing addCommand function
    addCommand(c, runInBackground);
}

/// Appends string s to the Options options and returns it
Options appendOption(char *s, Options options, int isOption) {
    options.arr = realloc(options.arr, (++(options.len) + 1) * sizeof(char *));

    // If s is a string, we must ignore the opening and closing quotation marks
    if (isOption) {
        options.arr[options.len-1] = malloc((strlen(s) - 1) * sizeof(char));

        memcpy(options.arr[options.len-1], &s[1], strlen(s)-2);
        options.arr[options.len-1][strlen(s)-2] = '\0';
    } else {
        options.arr[options.len-1] = strdup(s);
    }

    options.arr[options.len] = NULL;
    return options;
}

/// Appends a CommandPart cp to CommandList cl and returns it
CommandList appendCommandList(CommandPart cp, CommandList cl) {
    cl.cp = realloc(cl.cp, (++cl.len) * sizeof(CommandPart));
    cl.cp[cl.len-1] = cp;
    return cl;
}

/// Creates and starts a process, given a CommandPart and the in/out fd
int createProcess(int inFd, int outFd, CommandPart *cp) {
    pid_t pid;
    if ((pid = fork ()) == 0) {
        // Use inFd if it is set
        if (inFd != STDIN_FILENO) {
            dup2(inFd, STDIN_FILENO);
            close(inFd);
        }
        // Use outFd if it is set
        if (outFd != STDOUT_FILENO) {
            dup2(outFd, STDOUT_FILENO);
            close(outFd);
        }
        // Execute the command
        execvp(cp->options.arr[0], cp->options.arr);
        // Return an error
        _exit(255);
    } else {
        int stat = 0;
        waitpid(pid, &stat, 0);
        // Check whether the child returned an error
        return WIFEXITED(stat) && WEXITSTATUS(stat) != 255;
    } 

    return 0;
}

/// Run all commands in a pipe
void runPipe(CommandList cl, int inFd, int outFd) {
    pid_t pid = 0;
    int fd[2]; // 0 = read, 1 = write
    int errorFree = 1;
    
    // Execute each CommandPart in CommandList until the end, or until an error has occurred in the execution of an intermediate command
    for (int i = 0; errorFree && i < cl.len - 1; i++) {
        if (inputLine.errorOccurred) {
            return;
        }
        pipe(fd);
        errorFree = createProcess(inFd, fd[1], &cl.cp[i]);
        close(fd[1]);
        inFd = fd[0];
    }

    if (inFd != STDIN_FILENO) {
        dup2(inFd, STDIN_FILENO);
    }

    // Error found
    if (!errorFree) {
        printf("Error: command not found!\n");
        exit(EXIT_FAILURE);
    }

    // Reset the fds
    int outFd2 = dup(STDOUT_FILENO);
    dup2(inFd, STDIN_FILENO);
    dup2(outFd, STDOUT_FILENO);

    // Execute the last command
    Options options = cl.cp[cl.len - 1].options;
    if (execvp(options.arr[0], options.arr) < 0) {
        dup2(outFd2, STDOUT_FILENO);
        printf("Error: command not found!\n");
        inputLine.errorOccurred = 1;
        exit(EXIT_FAILURE);
    }
}

/// Run the commands in CommandList cl on a separate process
void runCommandList(CommandList cl, int inFd, int outFd, int runInBackground) {
    pid_t pid;
    if ((pid = fork()) == 0) {
        runPipe(cl, inFd, outFd);
    } else {
        if (!runInBackground) {
            waitpid(pid, NULL, 0);
        }
    }
}

/// Runs a given Command c
void runCommand(Command c) {
    // Initialise the in/out file descriptor to stdin/stdout respectively
    int inFd = STDIN_FILENO;
    int outFd = STDOUT_FILENO;

    // If the user gave an input IO redirection path, we use this for the in fd
    if (c.io.input != NULL) {
        inFd = open(c.io.input, O_RDONLY);
    }

    // If the user gave an output IO redirection path, we use this for the out fd
    if (c.io.output != NULL) {
        outFd = open(c.io.output, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    }

    // Run the CommandList for the current Command
    runCommandList(c.cl, inFd, outFd, c.runInBackground);

    // Close the fds if they were set in the above if-statements
    if (inFd != STDIN_FILENO) close(inFd);
    if (outFd != STDOUT_FILENO) close(outFd);
}

/// Runs the InputLine. Only does this if no error has occurred so far
void runInput() {
    if (!inputLine.errorOccurred) {
        for (int i = 0; i < inputLine.len; i++) {
            runCommand(inputLine.commands[i]);
        }
    }
}

/// Frees the Options by freeing each option, followed by freeing the array that held it
void freeOptions(Options a) {
    for (int i = 0; i < a.len; i++) {
        free(a.arr[i]);
    }
    free(a.arr);
}

/// Frees the CommandPart by freeing its command followed by freeing its Options
void freeCommandPart(CommandPart cp) {
    free(cp.command);
    freeOptions(cp.options);
}

/// Frees the CommandList by freeing each CommandPart followed by the array that holds it
void freeCommandList(CommandList cl) {
    for (int i = 0; i < cl.len; i++) {
        freeCommandPart(cl.cp[i]);
    }
    free(cl.cp);
}

/// Frees the IO
void freeIO(IO io) {
    free(io.input);
    free(io.output);
}

/// Frees the Command by freeing its CommandList and IO
void freeCommand(Command c) {
    freeCommandList(c.cl);
    freeIO(c.io);
}

/// Frees the InputLine by freeing all its Commands
void freeInput() {
    for (int i = 0; i < inputLine.len; i++) {
        Command c = inputLine.commands[i];
        freeCommand(c);
    }
    
    free(inputLine.commands);
}

/// Resets the input, making it ready for the next InputLine
void resetInput() {
    freeInput();
    newInput();
}