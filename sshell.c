#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16

struct command
{
    char *input;                      //contains full user command input
    char *args[ARGS_MAX];             //contains command and argument
    int count;
};

struct command readParse(char *cmd)
{
    struct command command;
    char *nl;
    char *token;
    char *cmdCopy = malloc(CMDLINE_MAX * sizeof(char));   //can be used to copy user command input

    /* Get command line */
    fgets(cmd, CMDLINE_MAX, stdin);

    /* Print command line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
        printf("%s", cmd);
        fflush(stdout);
    }

    /* Remove trailing newline from command line */
    nl = strchr(cmd, '\n');
    if (nl) {
        *nl = '\0';
    }

    /* Split command line into arguments by the white space tokens */
    command.count = 0;
    strcpy(cmdCopy, cmd);
    token = strtok(cmdCopy, "\n");
    command.input = token;
    token = strtok(cmd, " ");                             //create tokens when white spaces are present
    command.args[0] = token;
    while (token != NULL) {
        token = strtok(NULL, " ");
        command.args[command.count + 1] = token;
        command.count++;
    }
    return command;
}

// custom parse the command for pipe commands usage
void pipeParse(struct command *pipeCommand, char *string)
{
    char *token = strtok(pipeCommand->input, string);

    pipeCommand->count = 0;
    pipeCommand->args[pipeCommand->count] = token;
    pipeCommand->count++;
    while (token != NULL) {
        token = strtok(NULL, string);
        pipeCommand->args[pipeCommand->count] = token;
        pipeCommand->count++;
    }
}

// count number of pipes in a command
int pipeCount(char *cmd)
{
    int count = 0;

    for (int i = 0; i < strlen(cmd); ++i) {
        if (cmd[i] == '|') {
            count++;
        }
    }
    return count;
}

void pipeHandler(char **args, int maxCmd)
{
    int currCmd = 0;
    int count = 0;
    int out_redirection_file;
    
    // go through the pipe commands from left to right
    while (currCmd < maxCmd) {
        
        // if current command is the last pipe command then
        // check for output redirection       
        if (currCmd == maxCmd - 1) {
            struct command cmd;

            cmd.input = malloc(strlen(args[currCmd]) * sizeof(char));
            strcpy(cmd.input, args[currCmd]);
            pipeParse(&cmd, " ");
            
            //output redirection 
            while ((cmd.args[count] != NULL) {
                if (!strcmp(cmd.args[count], ">")) {
                    out_redirection_file = open(cmd.args[count + 1], O_RDWR | O_CREAT | O_TRUNC, 0644);
                    dup2(out_redirection_file, STDOUT_FILENO);
                    close(out_redirection_file);
                    cmd.args[count] = NULL;
                    break;
                } else if (!strcmp(cmd.args[count], ">>")) {
                    out_redirection_file = open(cmd.args[count + 1], O_RDWR | O_CREAT | O_APPEND, 0644);
                    dup2(out_redirection_file, STDOUT_FILENO);
                    close(out_redirection_file);
                    cmd.args[count] = NULL;
                    break;
                }
                count++;
            }
            execvp(cmd.args[0], cmd.args);
            perror("execvp error");

        }
        // if current command is not the last
        // pipe and fork
        if (currCmd < maxCmd) {
            int fd[2];
            pid_t pid;

            if (pipe(fd) < 0) {
                perror("pipe");
            }

            pid = fork();
            // if child, increment command counter
            if (pid == 0) {
                if (currCmd < maxCmd) {
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[0]);
                }
                close(fd[1]);
                currCmd++;
            }
            // if parent, execute command
            else if (pid > 0) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);

                struct command cmd;

                cmd.input = malloc(strlen(args[currCmd]) * sizeof(char));
                strcpy(cmd.input, args[currCmd]);
                pipeParse(&cmd, " ");
                execvp((&cmd)->args[0], (&cmd)->args);
                perror("execvp");
            } 
            else {
                perror("fork");
                exit(1);
            }
        }
    }
}

/* pwd() function to get the current working directory */
void pwd(struct command command)
{
    if (!strcmp(command.input, "pwd")) {    // if the command is "pwd"
        char cwd[CMDLINE_MAX];

        getcwd(cwd, CMDLINE_MAX);           // getcwd() function to get current directory from cwd
        printf("%s\n", cwd);
        fprintf(stderr, "+ completed '%s' [0]\n",
                command.input);
    }
}

/* cd() function to change the current working directory to specified directory */
void cd(struct command command)
{
    if (!strcmp(command.args[0], "cd")) {   // if the command is "cd"
        int error = chdir(command.args[1]); // chdir() function to change current directory to args[1]
        int status = 0;

        if (error == -1) {                  // if we cannot change to path specified in args[1], print error message
            fprintf(stderr, "Error: cannot cd into directory\n");
            status = 1;
        }
        fprintf(stderr, "+ completed '%s' [%d]\n",
                command.input, status);
    }
}

/* command_error() function to check parsing errors present in output redirection command */
int command_error(struct command command)
{
    int count = 0;

    /* using similar while loop as main() function to get parsing error from out redirection command */
    while (command.args[count] != NULL) {
        if (!strcmp(command.args[count], ">")) {
            if (command.args[count + 1] == NULL) {      // if the argument after ">" is empty
                return 1;                               // error: no output file
            }

            int out_redirection_file = open(command.args[count + 1], O_RDWR | O_CREAT | O_APPEND, 0644);
            close(out_redirection_file);

            if (out_redirection_file == -1) {           // if cannot access file specified after ">"
                return 2;                               // error: cannot open output file
            }
        }
        count++;
    }
    return 0;
}

int main(void)
{
    char *cmd = malloc(CMDLINE_MAX * sizeof(char));
    struct command command;                   // struct that contains user command and argument
    struct command pipeCommand;               // struct that contains user command and argument when there is piping
    int cmdError;
    int numPipes;                             // number of "|"
    int execError;

    while (1) {
        pid_t pid;
        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        command = readParse(cmd);             // parse command line with white spaces
        cmdError = command_error(command);    // check if there is any parsing error caused by out redirection
        numPipes = pipeCount(command.input);  // count number of pipes in command line

        if (command.count == ARGS_MAX) {
            fprintf(stderr, "Too Many Arguments\n");
            fflush(stdout);
            break;
        }

        /* Check parsing errors, builtin command, and regular command */
        if (cmdError == 1) {
            fprintf(stderr, "Error: no output file\n");
        } else if (cmdError == 2) {
            fprintf(stderr, "Error: cannot open output file\n");
        } else if (!strcmp(command.input, "exit")) {
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed '%s' [0]\n",
                    command.input);
            break;
        } else if (!strcmp(command.args[0], "pwd")) {
            pwd(command);
        } else if (!strcmp(command.args[0], "cd")) {
            cd(command);
        } else {
            /* fork the process and create parent process and child process */
            pid = fork();
            int status;
            if (pid == 0) {                  // child process
                int count = 0;
                int out_redirection_file;

                if (numPipes > 0) {          // if there is piping
                    numPipes = numPipes + 1;
                    pipeCommand.input = malloc(CMDLINE_MAX * sizeof(char));
                    strcpy(pipeCommand.input, command.input);
                    pipeParse(&pipeCommand, "|");
                    pipeHandler(pipeCommand.args, numPipes);
                } else {
                    /* two while loops to check ">" or ">>" in command line, then perform truncate or append */
                    while (command.args[count] != NULL) {
                        if (!strcmp(command.args[count], ">")) {
                            out_redirection_file = open(command.args[count + 1], O_RDWR | O_CREAT | O_TRUNC, 0644);
                            dup2(out_redirection_file, STDOUT_FILENO);
                            close(out_redirection_file);
                            command.args[count] = NULL;
                            break;

                        } else if (!strcmp(command.args[count], ">>")) {
                            out_redirection_file = open(command.args[count + 1], O_RDWR | O_CREAT | O_APPEND, 0644);
                            dup2(out_redirection_file, STDOUT_FILENO);
                            close(out_redirection_file);
                            command.args[count] = NULL;
                            break;
                        }
                        count++;
                    }

                    /* execute regular command line with execvp() */
                    execError = execvp(command.args[0], command.args);

                    /* print error message if execvp() fails to execute command */
                    if (execError == -1) {
                        fprintf(stderr, "Error: Command not found\n");
                    }
                    exit(1);
                }
            } else if (pid > 0) {            // parent process
                waitpid(-1, &status, 0);     // wait for child to finish, then print completion
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        command.input, status);
            } else {
                perror("fork");
            }
        }
    }
    return EXIT_SUCCESS;
}
