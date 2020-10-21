#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


#define CMDLINE_MAX 512

struct command {
    char *argument[16];
    char *commands[32];
    int count;
};

int parse (struct command *cmd) {
    char buffer[CMDLINE_MAX];
    char *temp = malloc(CMDLINE_MAX);

    /* Get command line */
    fgets(buffer, CMDLINE_MAX, stdin);

    /* Print command line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
        printf("%s", buffer);
        fflush(stdout);
    }

    cmd->commands[0] = strtok(strcpy(temp, buffer), "\n");

    cmd->count = 0;
    char* token = strtok(cmd->commands[0], " ");
    cmd->argument[cmd->count] = token;
    while (token != NULL) {
        token = strtok(NULL, " ");
        cmd->argument[cmd->count + 1] = token;
        cmd->count ++;
    }

    return 0;

}

int buildin (struct command *cmd) {
    if (!strcmp(cmd->argument[0], "pwd")) {
        char cwd[CMDLINE_MAX];
        getcwd(cwd, CMDLINE_MAX);
        printf("%s\n", cwd);
        fprintf(stderr, "+ completed '%s' [0]\n",
                cmd->commands[0]);
    }
    else if (!strcmp(cmd->argument[0], "cd")){
        int error = chdir(cmd->argument[1]);
        int status = 0;
        if (error == -1){
            fprintf(stderr, "Error: cannot cd into directory\n");
            status = 1;
        }
        fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd->commands[0], status);

    }
    else if (!strcmp(cmd->argument[0], "exit")) {
        fprintf(stderr, "Bye...\n");
        fprintf(stderr, "+ completed '%s' [0]\n",
                cmd->commands[0]);
        exit(0);
    }
    return 0;
}

int num(struct command *cmd){
    if ((!strcmp(cmd->argument[0], "pwd")) | (!strcmp(cmd->argument[0], "cd")) |
        (!strcmp(cmd->argument[0], "exit"))){
        return 1;
    } else {
        return 0;
    }
}

int main(void) {

    while (1) {
        pid_t pid;
        printf("sshell@ucd$ ");
        fflush(stdout);
        struct command c;

        parse(&c);

        if (num(&c) == 1) {
            buildin(&c);
        } else {
            pid = fork();
            int status = 0;
            if (pid == 0) {
                int error = execvp(c.argument[0], c.argument);

                if (error == -1){
                    fprintf(stderr, "Error: Command not found\n");
                }
		        exit(1);
            } else {
                if (waitpid(pid, &status, 0) < 0) {
                    break;
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", c.commands[0],status);

            }
        }
    }
    // code unreachable?
    return EXIT_SUCCESS;
}

