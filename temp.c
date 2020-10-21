#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16


struct command {
    char *input;  // full command input
    char *args[ARGS_MAX];
    int count;
};

struct command readParse(char* cmd){
    struct command command;
    char *nl;

    /* Get command line */
    fgets(cmd, CMDLINE_MAX, stdin);

    /* Print command line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
        printf("%s", cmd);
        fflush(stdout);
    }

    /* Remove trailing newline from command line */
    nl = strchr(cmd, '\n');
    if (nl)
        *nl = '\0';

    command.input = cmd;
    char* token = strtok(cmd," ");
    command.args[0] = token;
    command.count = 1;
    while(token != NULL){
        token = strtok(NULL, " ");
        command.args[command.count] = token;
        command.count++;
    }

    return command;
}

void pwd(struct command command) {
    if (!strcmp(command.input, "pwd")) {
        char cwd[CMDLINE_MAX];
        getcwd(cwd, CMDLINE_MAX);
        printf("%s\n", cwd);
        fprintf(stderr, "+ completed '%s' [0]\n", command.input);

    }
}

void cd(struct command command) {
    if (!strcmp(command.args[0], "cd")){
        int error = chdir(command.args[1]);
        int status = 0;
        if (error == -1){
            fprintf(stderr, "Error: cannot cd into directory\n");
            status = 1;
        }

        fprintf(stderr, "+ completed '%s' [%d]\n", command.input, status);
    }
}

int main(void)
{
    char *cmd = malloc(CMDLINE_MAX*sizeof(char));
    struct command command;
    while (1) {
        pid_t pid;
        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        command = readParse(cmd);
        command.args[command.count++] = NULL;

        /* Builtin command */
        if (!strcmp(command.input, "exit")) {
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed '%s' [0]\n", command.input);
            break;
        } else if (!strcmp(command.args[0], "pwd")){
            pwd(command);
        } else if (!strcmp(command.args[0], "cd")) {
            cd(command);
        } else {
            pid = fork();
            int status = 0;
            if (pid == 0) {
                int count = 0;
                int out_redirection_file = 0;
                while (command.args[count] != NULL) {
                    if (!strcmp(command.args[count], ">")) {
                        out_redirection_file = open(command.args[count + 1], O_RDWR|O_CREAT|O_TRUNC, 0644);
                        dup2(out_redirection_file,STDOUT_FILENO);
                        close(out_redirection_file);
                        command.args[count] = '\0';
                        break;

                    }
                    count++;
                }

                int error = execvp(command.args[0], command.args);

                if (error == -1){
                    fprintf(stderr, "Error: Command not found\n");
                }
                exit(1);
            } else {
                if (waitpid(pid, &status, 0) < 0) {
                    break;
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", command.input,status);

            }
        }

    }

    return EXIT_SUCCESS;
}
