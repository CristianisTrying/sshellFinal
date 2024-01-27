920919051, cehjimenez@ucdavis.edu
917532344, vnbarocio@ucdavis.edu

# sshell.c Program
## Overview
The objective of this project is to implement a simple shell which provides users with built-in commands:
cd, pwd, exit, sls; and non-built-in commands which support output redirection and piping. This was fulfilled
with the use of systemcalls that fall into these categories for programmers: files, pipes and processing. There
also exists the producing of our own functions in order to properly execute our shell. 

## Design
### Error Handling
For error handling, we have different ways to handle different types of commands. The types of commands that we have to handle are empty commands, built-in commands and non-built-in commands. The way we check for empty commands is by checking for an empty command line or an 
incorrect use of pipes in piping. For built-in commands, we use methods that find those built-in commands in the user-input. In addition, 
display the correct error messages when they are not found. For non-built-in commands, there are constraints such as a maximum number of 
arguments and a maximum number of pipes along with file error handling. Lastly, we ensure that child processes are executed before returning 
to the parent process

### Process Built-in Functions
For built-in commands, we have implemented the exit command, pwd, cd, and sls. The way we implemented these was by using functions that are
already in libraries and adding more code to it to make it work how it should. 

### Process Non-Built-in Functions
In handling non-built-in functions, the sshell program employs a well-structured approach. Command parsing and execution involve efficient tokenization through the parseCommandLine parser, addressing complexities like output redirection and pipes. Key functions such as getPipeCount and commandPipe manage execution intricacies, ensuring robust error handling for constraints like maximum arguments and pipes. Background execution, clear feedback, and security measures are seamlessly integrated. The program's design emphasizes scalability, making it adaptable to future enhancements while maintaining codebase extensibility.

## Structure
To make our functions run smoothly, we added a struct in order to group our variables into one place and be able
to access them efficiently. The aforementioned functions are listed as follows:
int attachPipe(int inputFileDirection, int outputFileDirection, struct cmdLine *cmd);
int commandPipe(int pipeCount, struct cmdLine *command, int returnValue[]);
int checkCommandValidity(char *cmd, char *cmd0);
int checkRedirectMislocation(char *cmd);
void parseCommandLine(char *cmd, struct cmdLine *command);
int getPipeCount(char *cmd);
void executeNonBuiltInCommands(char *cmd, int numPipeSects, struct cmdLine *commandList);

The attachPipe function is meant to take all the pipes on the command line and join them together in order to properly execute the command. 
The commandPipe function is meant to execute the commands that each pipe has set to do accordingly and in an orderly fashion. 
The checkCommandValidity function is meant to check to see if there is no trailing pipe or output redirections symbol. In the case that there
is, it will print out an error message. 
The checkRedirectMislocation function is meant to check to see if the pipe command does not come after the redirection symbol. This will cause
an error if done so and will print out an error message. 
The parseCommandLine function is meant to parse command line into cmdLine data structure so that the proper commands and files are accounted
for upon execution of each one. 
The getPipeCount function is meant to keep track of all the pipes in the command line string by parsing. 
The executeNonBuiltInCommands function is meant to make an elegant call in the int main to reduce the clutter of code in the main function.
