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
#define TOKEN_MAX 32

struct command {
  char *input;  // full user command input
  char *args[ARGS_MAX];   //args[0] = command for execvp
  int count;
  int numPipes;
};

struct command readParse(char *cmd) {
  struct command command;
  char *nl;
  char *token;
  char *cmdCopy = malloc(CMDLINE_MAX * sizeof(char));

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

  command.numPipes = 0;
  for (int i = 0; i < strlen(cmd); ++i) {
    if (cmd[i] == '|') {
      command.numPipes++;
    }
  }

  command.count = 0;
  strcpy(cmdCopy, cmd);
  token = strtok(cmdCopy, "\n");
  command.input = token;
  token = strtok(cmd, " ");
  command.args[0] = token;

  while (token != NULL) {
    token = strtok(NULL, " ");
    command.args[command.count + 1] = token;
    command.count++;
  }
  return command;
}

struct command parsePipe(struct command command) {
  struct command pipeCommand;

  pipeCommand.count = 0;
  pipeCommand.input = command.input;
  pipeCommand.numPipes = command.numPipes;

  char *token = strtok(command.input, "|");
  pipeCommand.args[pipeCommand.count] = token;
  pipeCommand.count++;
  while (token != NULL) {
    token = strtok(NULL, "|");
    pipeCommand.args[pipeCommand.count] = token;
    pipeCommand.count++;
  }
  return pipeCommand;
}

void pipeHandler(struct command pipeCommand) {
  int maxCommand = pipeCommand.numPipes;
  int currCommand = 0;
  int status;
  while (currCommand < maxCommand) {
    // last command, handle redirection
    if (currCommand == maxCommand) {
      struct command command;
      command = readParse(pipeCommand.args[currCommand]);
      command.args[command.count++] = NULL;
      execvp(command.args[0], command.args);
      fprintf(stderr, "+ completed '%s' : [%d]\n", command.input, status);
    }
    if (currCommand < maxCommand) {
      int fd[2];
      pid_t pid;

      if (pipe(fd) < 0) {
        perror("pipe");
      }

      pid = fork();
      if (pid > 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        struct command command;
        command = readParse(pipeCommand.args[currCommand]);
        command.args[command.count++] = NULL;
        execvp(command.args[0], command.args);
        waitpid(pid, &status, 0);
        fprintf(stderr, "+ completed '%s' : [%d]\n", command.input, status);
      }
      else if(pid == 0) {
        if (currCommand < maxCommand) {
          dup2(fd[0], STDIN_FILENO);
          close(fd[0]);
        }
        close(fd[1]);
        currCommand++;
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

int command_error(struct command command) {
  int count = 0;
  while (command.args[count] != NULL) {
    if (!strcmp(command.args[count], ">")) {
      if (command.args[count + 1] == NULL) {
        return 1;
      }
      int out_redirection_file = open(command.args[count + 1], O_RDWR | O_CREAT | O_APPEND, 0644);
      close(out_redirection_file);
      if (out_redirection_file == -1) {
        return 2;
      }
    }
    count++;
  }
  return 0;
}
int main(void) {
  char *cmd = malloc(CMDLINE_MAX * sizeof(char));
  struct command command;
  struct command pipeCommand;
  while (1) {
    pid_t pid;
    /* Print prompt */
    printf("sshell@ucd$ ");
    fflush(stdout);

    command = readParse(cmd);

    int error_num = command_error(command);

    if (command.count++ == 17) {
      fprintf(stderr, "Too Many Arguments\n");
      continue;
    }
    command.args[command.count++] = NULL;

    /* Builtin command */
    if (error_num == 1) {
      fprintf(stderr, "Error: no output file\n");
    } else if (error_num == 2) {
      fprintf(stderr, "Error: cannot open output file\n");
    } else if (!strcmp(command.input, "exit")) {
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
        int out_redirection_file;
        if (command.numPipes != 0) {
          pipeCommand = parsePipe(command);
          pipeHandler(pipeCommand);
        } else {
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

          int error = execvp(command.args[0], command.args);

          if (error == -1) {
            fprintf(stderr, "Error: Command not found\n");
          }
          exit(1);
        }
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
