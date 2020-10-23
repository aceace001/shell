# **ECS150 Project1 Report**

## **Simple Shell Overview**
We started our project by using the skeleton code provided to implement parsing
and builtin commands. We successfully implemented some of the main features 
including three builtin commands ("cd", "pwd", "exit"), regular commands ("ls",
"date", "echo", etc) and output redirection. We also made piping and error 
management partially working. One of the extra features "output redirection 
appending" is also implemented, but we didn't finish the other extra feature 
"ls-like builtin command".

## **Phase 0-3**
From phase 0 to 3, we were inspired by the skeleton code to implement parsing, 
builtin and regular command.

### Parsing command line
* In function 'readParse()', it will read user command input, create tokens when 
there are white spaces present in the command line by using 
'strtok(command," ")', and then split the full command line into arguments by 
tokens.

### Builtin commands
Before redirection and piping, we first check if the command is one of the
builtin command by using 'strcmp()' function.

* cd: in function 'cd()', we used 'chdir()' functon to change the current 
directory to the path specified. If we cannot change to the path by using 
'chdir()', then print error. 

* pwd: in function 'pwd()', we used 'getcwd()' function to get the current 
working directory. 

* exit: in 'main()' function, if the command is 'exit', we exit with status 0.

### Regular commands
* After checking builtin command and errors from redirection, we implement the ability for the shell to execute 
simple regular command such as "ls", "date", or "echo hello" by forking the 
process to create parent and child process. The parent wait until child process 
is finished by using 'waitpid()' function and print completion. The child 
executes regular command by using 'execvp()' function so that it will 
automatically search progtams in the '$PATH'. 

## **Phase 4-6**
From phase 4 to 6, we made changes to the implementation we had for phase 0-3 to
perform out redirection, piping, error management, and one extra feature "out 
redirection append"

### Out redirection
* In the 'main()' function, we perform output redirection inside child process. 
We use while loop to check each argument on the command line, if the charactor 
">" is spotted by using "!strcmp()", then open the file specified at the 
argument located after the argument ">". We write the command before ">" to the 
specified file using file control option "O_RDWR", and truncate it using 
"O_TRUNC". * Use 'dup2()' to connect stdout with the output file, then remove 
">" and filename in order for them to not be printed as output. 

### Out redirection append
* Similar logic as out redirection command with truncation. If ">>" is detected,
then write to the file specified with file control option "O_APPEND" instead of 
"O_TRUNC" in order to append the output to the file. Everything else is 
basically unchanged. 

### Piping


### Error management
* parsing errors: 
  * "no output file": we created another function "command_error()" to see if 
  the argument after ">" is empty by using the logic as "out redirection" 
  implementation. 
  * "cannot open output file": in this same function, if we cannot access the 
  file after the argument ">", then print error. 

* launching errors:
  * "Error: cannot cd into directory": in 'cd()' fucntion, print error if 
  'chdir()' fails.
  * "Error: command not found": if command is not found by 'execvp()' in the 
  child process, then print error message. However, we were not be able to print
  status value 1 for this error, instead we get a value of 256.
  
* Unfortunately we failed to implement other command errors. 

### ls-like builtin command
we failed to implement. 
