// Headers
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Macros defined
#define CMDLINE_MAX 512
#define ARGS_MAX 17
#define PIPES_MAX 4

// Structure to hold command line object
struct cmdLine {
  char *stringCmd;
  char *argsArray[ARGS_MAX];
  int numArgs;
  int outputRedirection;
  int extraSymbol;
  char *redirectFile;
  int redirectFileAccess;
  int runBackground;
};

// Function declarations
int attachPipe(int inputFileDirection, int outputFileDirection, struct cmdLine *cmd);
int commandPipe(int pipeCount, struct cmdLine *command, int returnValue[]);
int checkCommandValidity(char *cmd, char *cmd0);
int checkRedirectMislocation(char *cmd);
void parseCommandLine(char *cmd, struct cmdLine *command);
int getPipeCount(char *cmd);
void executeNonBuiltInCommands(char *cmd, int numPipeSects, struct cmdLine *commandList);

// Function used to connect all the pipes in the command line
int attachPipe(int inputFileDirection, int outputFileDirection, struct cmdLine *cmd) {

  int pid = fork();
  if (pid != 0) 
  {
    if (inputFileDirection != 0) 
    {
      dup2(inputFileDirection, STDIN_FILENO); // copy inputFileDirection to standard input
      close(inputFileDirection);
    }

    dup2(outputFileDirection, STDOUT_FILENO); // copy outD to standard ouput
    close(outputFileDirection);

    int returnedResult = execvp(cmd->stringCmd, cmd->argsArray);
    if (returnedResult == -1) // if exectue command fails, print out the error message
    {
      fprintf(stderr, "Error: command not found\n");
      return -1;
    }
  }

  return 0;
}

int commandPipe(int pipeCount, struct cmdLine *cmd, int retValues[]) {
  int inputFileDirection, outputFileDirection;
  int pipeFD[2];
  int i;

  inputFileDirection = 0;

  // loop through the commandline to set up the piping process
  for (i = 0; i < pipeCount - 1; i++) {
    pipe(pipeFD);
    outputFileDirection = pipeFD[1];
    retValues[i] = attachPipe(inputFileDirection, outputFileDirection, &cmd[i]);

    // check to see if it fails
    if (retValues[i] == -1) 
    {
      return 1;
    }

    close(pipeFD[1]);
    inputFileDirection = pipeFD[0];
  }

  // outputRedirection ouput for the last command if necessary
  dup2(inputFileDirection, STDIN_FILENO);
  close(inputFileDirection);

  if (cmd[pipeCount - 1].outputRedirection) // if there is a redirection, then we
                                       // have to open the file for redirection
  {
    if (cmd[pipeCount - 1]
            .extraSymbol) // if there is a file that exists, we put the string into
                     // that file, if not, we create a new one
    {
      outputFileDirection = open(cmd[pipeCount - 1].redirectFile,
                   O_WRONLY | O_CREAT | O_APPEND, 0644);
    } else {
      outputFileDirection = open(cmd[pipeCount - 1].redirectFile,
                   O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    dup2(outputFileDirection, STDOUT_FILENO);
    close(outputFileDirection);
  }

  retValues[pipeCount - 1] = execvp(cmd[i].stringCmd, cmd[i].argsArray);

  if (retValues[pipeCount - 1] == -1) {
    fprintf(stderr, "Error: command not found\n");
    return 1;
  }

  return 0;
}

// function ensures that there is no trailing pipe or output redirections symbol
int checkCommandValidity(char *command, char *keyword) {
  char commandCopy[CMDLINE_MAX];
  strcpy(commandCopy, command); // copy the command line

  int hasRedirection = 0;

  // checks for file after output redirection symbol
  if (strstr(command, ">")) {
    char *redirLocation = strstr(command, ">");
    if (strstr(command, keyword) > redirLocation) {
      hasRedirection = 1;
    }
  }
  // checks for command after pipe symbol
  if (strstr(command, "|")) {
    char *pipeLocation = strstr(command, "|");
    if (strstr(command, keyword) > pipeLocation) {
      hasRedirection = 1;
    }
  }

  return hasRedirection;
}

// ensures that the pipe command does not come after the redirection symbol
int checkRedirectMislocation(char *command) {
  int hasMislocatedRedirect = 0;

  if (strstr(command, ">")) {
    char *redirectLocation = strstr(command, ">");
    if (strstr(redirectLocation, "|")) {
      hasMislocatedRedirect = 1;
    }
  }

  return hasMislocatedRedirect;
}

// initializes the properties of the command line object
void initializeCommand(struct cmdLine *command) {
  command->numArgs = 0;
  command->outputRedirection = 0;
  command->extraSymbol = 0;
  command->redirectFile = NULL;
  command->redirectFileAccess = 1;
  command->runBackground = 0;
}

// parses command line into cmdLine data structure
void parseCommandLine(char *cmdStr, struct cmdLine *command) {
  char *holdCmdString;
  int i = 1;
  command->numArgs = 0;
  command->outputRedirection = 0;
  command->extraSymbol = 0;
  command->redirectFile = NULL;
  command->redirectFileAccess = 1;
  command->runBackground = 0;

  char *previousCmdString;

  // if command has outputRedirection symbol
  if (strstr(cmdStr, ">")) {

    command->outputRedirection = 1;

    char *redirLoc = strstr(cmdStr, ">");

    if (strstr(cmdStr, ">>")) {
      command->extraSymbol = 1;
    }

    // breaks string into before and after outputRedirection
    previousCmdString = strtok(cmdStr, ">");
    if (command->extraSymbol) {
      command->redirectFile = strtok(redirLoc + 2, " \n");
    } else {
      command->redirectFile = strtok(NULL, " \n");
    }

    // checks if file can be opened
    int fd = open(command->redirectFile, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
      command->redirectFileAccess = -1;
    }
    close(fd);
  }

  // tokes through string to get arguments
  if (command->outputRedirection) {
    holdCmdString = strtok(previousCmdString, " \n");
  } else {
    holdCmdString = strtok(cmdStr, " \n");
  }

  if (holdCmdString == NULL) {
    holdCmdString = "";
  }

  command->stringCmd = holdCmdString;
  command->argsArray[0] = holdCmdString;

  while (holdCmdString != NULL) {
    holdCmdString = strtok(NULL, " \n");
    if (holdCmdString != NULL) {
      if (holdCmdString[0] != ' ' && holdCmdString[0] != '\n') {
        command->argsArray[i] = holdCmdString;
        i++;
      } else {
        i++;
      }
    } else {
      command->argsArray[i] = holdCmdString;
    }
  }

  // check for background symbol
  if ((command->argsArray[i - 1])[strlen(command->argsArray[i - 1]) - 1] ==
      '&') {
    command->runBackground = 1;
    // alters last argument based on if just '&' or '&' attached to end of last
    // argument
    if ((command->argsArray[i - 1])[0] == '&') {
      command->argsArray[i - 1] = NULL;
    } else {
      int lastChar = strlen(command->argsArray[i - 1]) - 1;
      (command->argsArray[i - 1])[lastChar] = '\0';
    }
  }

  command->numArgs = i;
}

// function to get the number of pipes
// counts number of pipe sections
int getPipeCount(char *cmd) {
  char *pipeSection;
  int pipeCount = 0;

  pipeSection = strtok(cmd, "|");

  while (pipeSection != NULL) {
    pipeCount++;
    pipeSection = strtok(NULL, "|");
  }
  return pipeCount;
}
  int filter(const struct dirent *entry) {
    return strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..");
  }

void executeNonBuiltInCommands(char *cmd, int numPipeSects,
                               struct cmdLine *commandList) {
  int totalArgs = 0;
  // iterates through the argument array
  for (int i = 0; i < numPipeSects; i++) {
    totalArgs += commandList[i].numArgs;
  }

  // Error handling
  if (totalArgs > ARGS_MAX-1) {
    fprintf(stderr, "Error: too many process arguments\n");
  } else if (commandList[numPipeSects - 1].redirectFile == NULL &&
             commandList[numPipeSects - 1].outputRedirection) {
    fprintf(stderr, "Error: no output file\n");
  } else if (commandList[numPipeSects - 1].outputRedirection &&
             commandList[numPipeSects - 1].redirectFileAccess == -1) {
    fprintf(stderr, "Error: cannot open output file\n");
  } else if (checkRedirectMislocation(cmd)) {
    fprintf(stderr, "Error: mislocated output redirection\n");
  } else {
    int pid;
    int returnValue[PIPES_MAX];
    for (int i = 0; i < PIPES_MAX; i++) {
      returnValue[i] = 0;
    }

    pid = fork();
    // Parent waits for status and prints
    if (pid > 0) {
      int status;
      // parent process waits for status variable after child process is
      // executed
      if (commandList[numPipeSects - 1].runBackground) {
        waitpid(-1, &status, WNOHANG);
      } else {
        wait(&status);
      }

      // checks each index in the returnValue array and exits if one of the
      // cmd fail
      returnValue[numPipeSects - 1] = WEXITSTATUS(status);
      if (returnValue[numPipeSects - 1] == -1) {
        _Exit(EXIT_FAILURE);
      }
      // Print completion and exit status
      fprintf(stderr, "+ completed '%s' ", cmd);
      for (int i = 0; i < numPipeSects; i++) {
        fprintf(stderr, "[%d]", returnValue[i]);
      }
      fprintf(stderr, "\n");

    } else {
      _Exit(commandPipe(numPipeSects, commandList, returnValue));
    }
  }
}

int main(void) {
  char cmd[CMDLINE_MAX];

  while (1) {

    char *nl;

    /* Print prompt */
    printf("sshell@ucd$ ");
    fflush(stdout);

    /* Get commandList[0] line */
    fgets(cmd, CMDLINE_MAX, stdin);

    /* Print commandList[0] line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
      printf("%s", cmd);
      fflush(stdout);
    }

    /* Remove trailing newline from commandList[0] line */
    nl = strchr(cmd, '\n');
    if (nl)
      *nl = '\0';

    // Creates copy of commandList[0] line before toking
    char cmdStrCpy[CMDLINE_MAX];
    strcpy(cmdStrCpy, cmd);

    // Handles case where nothing entered, included becuase when empty
    // numPipeSects is 0
    if (!strcmp(cmd, "")) {
      continue;
    }

    // Handles case where just pipe is inputted
    if (!strcmp(cmd, "|") || cmd[strlen(cmd) - 1] == '|' ||
        cmd[0] == '|') {
      fprintf(stderr, "Error: missing command\n");
      continue;
    }

    // creates copy used to count pipe sects
    char cmdStrForPipeCheck[CMDLINE_MAX];
    strcpy(cmdStrForPipeCheck, cmd);
    int numPipeSects = getPipeCount(cmdStrForPipeCheck);

    struct cmdLine commandList[numPipeSects];

    char *restStr = cmdStrCpy;
    char *nextPipeSection = strtok_r(restStr, "|\n", &restStr);
    parseCommandLine(nextPipeSection, &commandList[0]);

    // loop through to break up pipe sections
    int i = 1;
    while (i < numPipeSects) {
      nextPipeSection = strtok_r(restStr, "|\n", &restStr);
      parseCommandLine(nextPipeSection, &commandList[i]);
      i++;
    }

    // checks for missing command error
    int missingCmd = 0;
    if (checkCommandValidity(cmd, commandList[0].stringCmd)) {
      missingCmd = 1;
    }
    for (int i = 0; i < numPipeSects; i++) {
      if (!strcmp(commandList[i].stringCmd, "") && !missingCmd) {
        missingCmd = 1;
      }
    }

    // Handles case where only spaces entered
    if (numPipeSects == 1 &&
        (commandList[0].stringCmd == NULL || !strcmp(commandList[0].stringCmd, ""))) {
      continue;
    }

    // print error message if missing command error found
    if (missingCmd) {
      fprintf(stderr, "Error: missing command\n");
      continue;
    }

    /* Builtin cmd */
    // exit
    if (!strcmp(cmd, "exit")) {
      fprintf(stderr, "Bye...\n");
      fprintf(stderr, "+ completed '%s' [0]\n", cmd);
      break;
    }
    // print working directory
    if (!strcmp(cmd, "pwd")) {
      char cwd[512];
      getcwd(cwd, 512);
      printf("%s\n", cwd);
      fprintf(stderr, "+ completed '%s' [0]\n", cmd);
      continue;
    }
    // change directory
    if (!strcmp(commandList[0].stringCmd, "cd")) {
      int ret = chdir(commandList[0].argsArray[1]);
      if (ret != 0) {
        fprintf(stderr, "Error: cannot cd into directory\n");
        fprintf(stderr, "+ completed '%s' [1]\n", cmd);
      } else {
        fprintf(stderr, "+ completed '%s' [0]\n", cmd);
      }
      continue;
    }

// sls command
if (!strcmp(commandList[0].stringCmd, "sls")) {
  struct dirent **eps;
  int n;


  n = scandir(".", &eps, filter, alphasort);
  if (n >= 0) {
    int cnt;
    for (cnt = 0; cnt < n; ++cnt)
      printf("%s\n", eps[cnt]->d_name);

    // Free allocated memory
    for (cnt = 0; cnt < n; ++cnt)
      free(eps[cnt]);
    free(eps);

    fprintf(stderr, "+ completed 'sls' [%d]\n", n);
    continue;
  } else {
    perror("Error: couldn't open the directory");
    fprintf(stderr, "+ completed 'sls' [1]\n");
    continue;
  }
}


    /* Non Built In Commands */
    int totalArgs = 0;
    for (int i = 0; i < numPipeSects; i++) {
      totalArgs += commandList[i].numArgs;
    }

    // error handling
    if (totalArgs > ARGS_MAX-1) {
      fprintf(stderr, "Error: too many process arguments\n");
    } else if (commandList[numPipeSects - 1].redirectFile == NULL &&
               commandList[numPipeSects - 1].outputRedirection) {
      fprintf(stderr, "Error: no output file\n");
    } else if (commandList[numPipeSects - 1].outputRedirection &&
               commandList[numPipeSects - 1].redirectFileAccess == -1) {
      fprintf(stderr, "Error: cannot open output file\n");
    } else if (checkRedirectMislocation(cmd)) {
      fprintf(stderr, "Error: mislocated output redirection\n");

      // execute command
    } else {
      int pid;
      int rets[PIPES_MAX];
      for (int i = 0; i < PIPES_MAX; i++) {
        rets[i] = 0;
      }

      pid = fork();
      // parent waits for status and prints
      if (pid > 0) {
        int status;
        // run in background or wait for status
        if (commandList[numPipeSects - 1].runBackground) {
          waitpid(-1, &status, WNOHANG);
        } else {
          wait(&status);
        }

        rets[numPipeSects - 1] = WEXITSTATUS(status);
        if (rets[numPipeSects - 1] == -1) {
          break;
        }
        // print completion and exit status
        fprintf(stderr, "+ completed '%s' ", cmd);
        for (int i = 0; i < numPipeSects; i++) {
          fprintf(stderr, "[%d]", rets[i]);
        }
        fprintf(stderr, "\n");

      } else {
        return commandPipe(numPipeSects, commandList, rets);
      }
    }
  }

  return EXIT_SUCCESS;
}