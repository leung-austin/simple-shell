## Project 1 - Simple shell 

To begin our project we started in phase 1 with the removal of system() and
implemented a simple fork(), execvp(), and wait() setup. Forking allowed for a 
parent to be able to wait for the child and the execution of the given command.
This structure we decided to keep and created into a function we have now 
called runSingleCommand (line ---). By giving the function a it will executed 
it and return the status of execvp. 

### Data Structures

To store each command we recieve a struct called Command is used. Command
contains an array of strings called args (arguments) and a single string called
command. As the cmd gets read in we parse the tokens of each command into this 
struct command assigning args to be each token and command to be only the 
first. (More on parsing later) For example if the read cmd was: 
"echo hello world", then args would be ["echo", "hello", "world", "Null"] 
while the command just stores "echo". 

Our second data structure called cmds is an array of the struct Command
described earlier. This is used to combine all the commands together to be 
able to later implement piping which involves multiple commands. 

### Parsing

Upon getting the command line as cmd a function called cleanCmd is called on
cmd. cleanCmd is used to clean up our input and set it up to work with some 
of our core functions. cleanCmd looks for spacing inconsistencies, for example
"echo hello> file.txt", here we need to determine that > is a call to redirect
and not an argument to echo. A space gets added before > to be consistent with 
spacing used for future functions. The same is done for piping symbols "|". 

Now that we have a consistent spacing format, another function
(splitArgsBySpace) is called. This function splits by whitespace the
tokens/arguments into an array. At this point we have a single array containing
each argument. 

Before placing the arguments into our Command data structure, we check to make
sure it is not actually multiple commands. This is done in a function called
argsToCmd. If any "|" symbols are found we know we have multiple commands and 
split them into seperate Command structs. Each struct is then placed into our
cmds struct which holds each Command. The same is for ">" as the redirect 
is split between the command and output destination. 

Our main data structure cmds is now setup and our parsing is done. 

### Builtin Commands 

Upon reading the input from cmd, in the main function we check for builtin 
commands by searching for there names. Looking for exit comes first as this 
will shut down our shell. Next we have pwd which uses the getcwd() function to 
retrieve the current directory. These are done before parsing our cmd by token 
to save time since exit may exit requiring us to not need to parse at all. 

cd works by first checking if the directory exists and then uses the chdir()
function to change directorys into the given path. 

### Extra Features 

sls is detected during parsing and is implemented in its our functon sls().
To start sls opens the current directory using opendir(). With the directory
open we first check if the directory exists then read each file. As the file is
read its written to stdout as well as its file size from stats.


### Output Redirection 

In our program, the output redirection function, redirect, takes in commands 
from the command line that were parsed by our function, cleanCmd. The function 
then takes these commands and calls upon fork, dup2, open, etc. to manipulate 
the stdout file descriptor prior to executing the command such that the program
prints the output into the desired file and not to the terminal. In addition, 
the redirect function handles the append feature based on one of the arguments 
that determine whether to truncate the command’s output or to append the 
output to the file.

### Pipeline 

In order to execute pipe commands, the parser, cleanCmd, determines the number 
of pipes in the command line. Once determined, the commands are sent to a 
separate function depending on the number of pipes to execute the piped 
commands. The function handles the creation of pipes and file descriptors for 
these pipes. Then, using the fork function to create child processes, dup2 is 
used to duplicate the stdin and stdout of the file descriptors into the pipes. 
Thus, allowing the communication between processes. In addition, our program 
has additional functions to handle the use of output redirection and pipeline 
commands together. 

### Testing

To test our shell we implemented a list of test cases aiming to hit every 
different type we can get. Each function was tested as well as commands of 
varying lengths. 


### Challenges 

The initial went smoothly, it was not until piping that we encountered issues. 
Figuring out how to connect multiple pipes was a challenge. We started with 
a simple 1 pipe with two commands and got that to work. However addition piping
was a challenge. We initally had one function for piping however ran into 
problems. With time running out we decided to have seperate functions for 
piping depending on how many were included. 

Our initial parser was also not prepared for the challenge of distinguising 
a redirect versus a redirect append. This is another challenge we faced and 
could improve. 

### Sources
For this project our sources included lecture material, and the GNU C Library 
to reference different commands. 



