# Mini shell
A small shell written in C, (F)lex and Bison. The shell has the following features:
- Start a program which can be found in the user’s search path (`$PATH`)
- I/O redirection, for example: `a.out < in > out`
- Background processes, for example: `a.out &`
- Pipes, for example: `a.out | b.out | c.out`
- String parsing, for example `a.out "some string <> with 'special' characters"`
- Any combination of the above, such as `sleep 5 & foo | bar < in > out`

# Files
- `Makefile`:           used make the project and generate the executable file `shell`
- `command.c`:          contains all the relevant C code used for creating, running and freeing the processes
- `command.h`:          header that contains all the structs and global variables
- `parser.y`:           the bison file
- `lexer.fl`:           the flex file

# Flow
First, we run the input through the flex file. In here, it tries to match characters with regular expressions and tokenizes them. When it finds a match, it will return the corresponding token, which will be used in the bison file (`parser.y`). 

An example:
```
{option}        { return(OPTION); }
```

In this line, if the input matched the regular expression `\"[^"]+\"` (which means, read an opening and closing " and read everything inbetween, except for a third ") and will return the OPTION token.

Once a token is returned, it will be used in the `parser.y`. In this file, we have simply defined the grammar as shown in the assignment. It takes quite a long time to explain the entire parser if you have no experience with Bison, so we will only do this for the first non-terminal Input:

```
Input           : InputLine {
                    if (!wantsToExit) {
                      runInput();
                    }
                    resetInput();
                  } NEWLINE Input
                | InputLine
                ;
```                

In this line, we define the transition rule:

```
Input -> InputLine NEWLINE Input
       | InputLine
```

which essentially allows us to constantly read new commands. As you might have noticed, we have added sections `{}` between `InputLine` and `NEWLINE` in our code. This simply allows us to run C code right after having read an `InputLine`. While parsing the `InputLine`, we store all the information we have read so far in a global variable `InputLine inputLine`, which is defined in `command.h`. 

If there is a syntax error found while parsing the `InputLine`, bison will call the yyerror function, which is defined at the top of the parser.y. If there is no syntax error, however, we will call the function `runInput` defined in `command.c` which will run all the commands in the `inputLine` variable and reset the variable afterwards. It continues doing this until the token `EXIT` was matched by the flex file `lexer.fl` and passed to the bison file `parser.y`.

# Important functions
Note that we have split out the functions to follow the grammar as much as possible. This means that there's a run and free function for each non-terminal defined in the grammar in the assignment. We will only explain the main run and free, since everything else is simply called following the same grammar and is trivial.

## runInput
This function takes all the information currently stored in the global variable `inputLine` and first checks whether any errors have occurred so far. If this is the case, none of the code is ran and we simply return back to the bison file, which will start parsing the next line of the input. If there are no erros, however, we iterate over all Commands in the `InputLine` and execute them one at a time.

## resetInput
This function resets the global variable `inputLine` by freeing everything first and then reallocating it, preparing it for the next line of the input.
