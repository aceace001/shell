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
#define PIPE_MAX 4

struct command {
  char *input;  // full user command input
  char *args[ARGS_MAX+1];   //args[0] = command for execvp
  int count;
};

struct pipeCommands {
  char *input;
  struct command commands[PIPE_MAX];
  int count;
};

struct command readParse(char *cmd) {
  struct command command;
  char *cmdCopy = malloc(CMDLINE_MAX * sizeof(char));

  strcpy(cmdCopy, cmd);
  char *token = strtok(cmdCopy, "\n");
  command.input = token;

  token = strtok(cmd, " ");
  command.args[0] = token;
  command.count = 1;
  while (token != NULL) {
    token = strtok(NULL, " ");
    command.args[command.count] = token;
    command.count++;
  }
  return command;
}

struct pipeCommands readPipeParse(char *cmd) {
  struct pipeCommands command;
  char *cmdCopy = malloc(CMDLINE_MAX * sizeof(char));
  int pipeCount = 0;

  strcpy(cmdCopy, cmd);
  char *token = strtok(cmdCopy, "\n");
  command.input = token;

  token = strtok(cmd, "|");
  command.commands[0].input = token;
  command.count = 1;
  while (token != NULL) {
    token = strtok(NULL, " ");
    command.commands[command.count].input = token;
    command.count++;
  }

  for (int i = 0; i < command.count; ++i) {
    struct command parsedCommand = readParse(command.commands[command.count].input);
    command.commands[command.count] = parsedCommand;
  }

  while (pipeCount < command.count) {
    if (pipeCount == command.count - 1) {
      /*
       * handle redirection here, only possible at end of pipe
       */
      command.commands[command.count].args[command.commands[command.count].count] = NULL;
      int error = execvp(command.commands[command.count].args[0], command.commands[command.count].args);

      if (error == -1) {
        fprintf(stderr, "Error: Command not found\n");
      }

    } else {
      int fd[2];
      pid_t pid;
      int status;

      if (pipe(fd) < 0) {
        perror("pipe");
      }
      pid = fork();
      if (pid > 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        command.commands[command.count].args[command.commands[command.count].count] = NULL;
        int error = execvp(command.commands[command.count].args[0], command.commands[command.count].args);

        if (error == -1) {
          fprintf(stderr, "Error: Command not found\n");
        }
        waitpid(pid, &status, 0);
      }
      else if (pid == 0){
        if (pipeCount != command.count - 1) {
          dup2(fd[0], STDIN_FILENO);
        }
        close(fd[1]);
        pipeCount++;
      }
      else {
        perror("fork");
      }
    }
  }
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
  if (!strcmp(command.args[0], "cd")) {
    int error = chdir(command.args[1]);
    int status = 0;
    if (error == -1) {
      fprintf(stderr, "Error: cannot cd into directory\n");
      status = 1;
    }
    fprintf(stderr, "+ completed '%s' [%d]\n", command.input, status);
  }
}

int main(void) {
  char *cmd = malloc(CMDLINE_MAX * sizeof(char));
  struct command command;
  struct pipeCommands pipeCommand;
  char *nl;
  while (1) {
    pid_t pid;
    /* Print prompt */
    printf("sshell@ucd$ ");
    fflush(stdout);

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

    //command = readParse(cmd);
    //command.args[command.count++] = NULL;

    pipeCommand = readPipeParse(cmd);

    /* Builtin command */
    if (!strcmp(command.input, "exit")) {
      fprintf(stderr, "Bye...\n");
      fprintf(stderr, "+ completed '%s' [0]\n", command.input);
      break;
    } else if (!strcmp(command.args[0], "pwd")) {
      pwd(command);
    } else if (!strcmp(command.args[0], "cd")) {
      cd(command);
    } else {
      pid = fork();
      int status;
      if (pid == 0) {
        int count = 0;
        int out_redirection_file = 0;
        while (command.args[count] != NULL) {
          if (!strcmp(command.args[count], ">")) {
            out_redirection_file = open(command.args[count + 1], O_RDWR | O_CREAT | O_TRUNC, 0644);
            dup2(out_redirection_file, STDOUT_FILENO);
            close(out_redirection_file);
            command.args[count] = NULL;
            break;

          }
          count++;
        }

        int error = execvp(command.args[0], command.args);

        if (error == -1) {
          fprintf(stderr, "Error: Command not found\n");
        }
        exit(1);
      } else {
        if (waitpid(pid, &status, 0) < 0) {
          break;
        }
        fprintf(stderr, "+ completed '%s' [%d]\n", command.input, status);

      }
    }

  }

  return EXIT_SUCCESS;
}
