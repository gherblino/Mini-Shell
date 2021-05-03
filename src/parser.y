%{
/**********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "command.h"

extern int yylex(void);
extern int yylex_destroy();
extern char *yytext;

/// Called when an error occurred. We only want to print invalid syntax if no other error has occurred so far
void yyerror(char const *msg) {
  if (inputLine.errorOccurred == 0) {
    printf("Error: invalid syntax!\n");
  }
}

/**********************************************/
%}
%code requires {
  #include "command.h"
}
%define parse.error verbose

%union {
  Command c;
  IO io;
  CommandList cl;
  CommandPart cp;
  Options op;

  char *s;
}

%type <c> Command
%type <io> IoRedirection
%type <cl> CommandList
%type <cp> CommandPart
%type <op> Option

%type <s> Text

%token COMMAND NEWLINE OPTION EXIT

%start Input

%%

Input           : InputLine {
                    if (!wantsToExit) {
                      runInput();
                    }
                    resetInput();
                  } NEWLINE Input
                | InputLine
                ;

InputLine       : Command '&' { if (strcmp(currentCommand, "exit") != 0) { addCommand($1, 1);} } InputLine
                | Command { if (strcmp(currentCommand, "exit") != 0) { addCommand($1, 0);} }
                | /* empty */
                ;

Command         : CommandList IoRedirection { Command c; c.cl = $1; c.io = $2; $$ = c; }

IoRedirection   : '<' Text '>' Text {
                    IO io;
                    io.input = $2;
                    io.output = $4;

                    if (strcmp($2, $4) == 0) {
                      printf("Error: input and output files cannot be equal!\n");
                      inputLine.errorOccurred = 1;
                    }

                    $$ = io;
                  }
                | '>' Text '<' Text {
                    IO io;
                    io.input = $4;
                    io.output = $2;
                    
                    if (strcmp($2, $4) == 0) {
                      printf("Error: input and output files cannot be equal!\n");
                      inputLine.errorOccurred = 1;
                    }

                    $$ = io;
                  }
                | '>' Text {
                    IO io;
                    io.input = NULL;
                    io.output = $2;
                    $$ = io;
                  }
                | '<' Text {
                    IO io;
                    io.input = $2;
                    io.output = NULL;
                    $$ = io;
                  }
                | {
                    IO io;
                    io.input = NULL;
                    io.output = NULL;
                    $$ = io;
                }
                ;

Text            : COMMAND { $$ = strdup(yytext); }

CommandList     : CommandList '|' CommandPart { $$ = appendCommandList($3, $1); }
                | CommandPart {
                    CommandPart cp = $1;
                    CommandList cl; 
                    cl.cp = malloc(sizeof(CommandPart)); 
                    cl.cp[0] = cp; 
                    cl.len = 1; 
                    $$ = cl;
                  }
                ;

CommandPart     : EXIT {
                    if (waitpid(-1, NULL, WNOHANG) == -1) {
                      freeInput();
                      yylex_destroy();
                      exit(EXIT_SUCCESS);
                    } else {
                      printf("Error: there are still background processes running!\n");
                      wantsToExit = 1;
                    }
                  }
                | Text { currentCommand = $1; } Option {
                    CommandPart cp;
                    cp.command = currentCommand;
                    cp.options = $3;
                    $$ = cp;
                  }
                ;

Option          : Option COMMAND { 
                      $$ = appendOption(yytext, $1, 0);
                    }
                | Option OPTION {
                      $$ = appendOption(yytext, $1, 1);
                    }
                | /* empty */ { 
                    Options options;
                    options.arr = malloc(sizeof(char *));
                    options.arr[0] = strdup(currentCommand);
                    options.len = 1;
                    $$ = options;
                  }
                ;
%%



/*****************************************************************/
/* the following code is copied verbatim in the generated C file */

int main() {
  // Initialise global InputLine inputLine
  newInput();

  setbuf(stdout, NULL);
  signal(SIGCHLD, SIG_IGN);

  // Continue parsing forever until manually stopped
  while(1) {
    yyparse();
  }
  
  // Clean up
  free(currentCommand);

  return 0;
}
/*****************************************************************/