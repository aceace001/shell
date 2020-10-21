#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16
#define TOKEN_MAX 32

struct command {
  char *input;  // full command input
  char *cmd;    // cmd = args[0]
  char *args[ARGS_MAX];
  int numOfArgs;
};

struct command readParse(char* cmd){
  struct command command;
  char *nl;
  char* token = malloc(TOKEN_MAX*sizeof(char));

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
  token = strtok(cmd," ");
  command.args[0] = token;
  command.numOfArgs = 1;
  while(token != NULL){
    token = strtok(NULL, " ");
    command.args[command.numOfArgs] = token;
    command.numOfArgs++;
  }
  command.cmd = command.args[0];
  return command;
}

void pwd(struct command command) {
  if (!strcmp(command.input, "pwd")) {
    char cwd[CMDLINE_MAX];
    getcwd(cwd, CMDLINE_MAX);
    printf("%s\n", cwd);
    fprintf(stderr, "+ completed '%s' [0]\n", command.input);
    exit(0);
  }
}

void cd(struct command command) {
  if (!strcmp(command.cmd, "cd")){
    int error = chdir(command.args[1]);
    int status = 0;
    if (error == -1){
      fprintf(stderr, "Error: cannot cd into directory\n");
      status = 1;
    }
    fprintf(stderr, "+ completed '%s' [%d]\n", command.input, status);
    exit(0);
  }
}

int main(void)
{
  char *cmd = malloc(CMDLINE_MAX*sizeof(char));
  struct command command;
  while (1) {
    int retval = 0;
    pid_t pid;
    /* Print prompt */
    printf("sshell@ucd$ ");
    fflush(stdout);

    command = readParse(cmd);
    command.args[command.numOfArgs++] = NULL;

    /* Builtin command */
    if (!strcmp(command.input, "exit")) {
      fprintf(stderr, "Bye...\n");
      fprintf(stderr, "+ completed '%s' [0]\n", command.input);
      break;
    }

    /* Regular command */
    pid = fork();
    if(pid == 0) {
      // pwd
      pwd(command);

      //cd
      cd(command);

      execvp(command.cmd, command.args);
      perror("execvp");
      exit(1);
    } else if (pid > 0) {
      int status;
      waitpid(pid, &status, 0);
    } else {
      perror("fork");
      exit(1);
    }
    //fprintf(stderr, "Return status value for '%s': %d\n", command.input, retval);
  }

  return EXIT_SUCCESS;
}



