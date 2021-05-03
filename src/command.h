#ifndef COMMAND_H
#define COMMAND_H

typedef struct Options {
    char **arr;
    int len;
} Options;

typedef struct CommandPart {
    char *command;
    Options options;
} CommandPart;

typedef struct CommandList {
    CommandPart *cp;
    int len;
} CommandList;

typedef struct IO {
    char *input;
    char *output;
} IO;

typedef struct Command {
    CommandList cl;
    IO io;

    int runInBackground;
} Command;

typedef struct InputLine {
    Command *commands;
    int len;

    int errorOccurred;
} InputLine;

InputLine inputLine;

char *currentCommand;

int wantsToExit;

void newInput();
void addCommand(Command c, int runInBackground);
void appendEmptyCommand(int runInBackground);
Options appendOption(char *s, Options options, int isOption);
CommandList appendCommandList(CommandPart cp, CommandList cl);
void runInput();
void freeInput();
void resetInput();

#endif