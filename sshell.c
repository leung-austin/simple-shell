
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512



/*
    struct for a single command.
    ex: echo hello world
      args would be ["echo", "hello", "world", NULL]
      command would be "echo"
*/
struct Command {
   char *args[17];
   char *command;
};

/*
  An array of commands
  ex: cmds[0].args = ["echo", "hello", "world", NULL]
      cmds[0].command = "echo"
*/
struct Command cmds[17];

struct Command redirectExtraArgs(struct Command cmd1, struct Command cmd2) {
	int len = sizeof(cmd1.args)/sizeof(cmd1.args[0]);
	int firstCmdCount = 0;
	int secondCmdCount = 0;
	for (int i = 0; i < len; i++) {
		if(cmd1.args[i] != NULL) {
			firstCmdCount++;
		}
		if(cmd2.args[i] != NULL) {
			secondCmdCount++;
		}
	}
	for(int i = 0; i < secondCmdCount -1; i++) {
		cmd1.args[firstCmdCount] = cmd2.args[i+1];
		firstCmdCount++;
	}
	return cmd1;
}


int sls()
{
    struct dirent *file;
    //From sys/stat.h
    struct stat stats;
    DIR *directoy;

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    directoy = opendir(cwd);
    if (directoy)
    {
          int numFiles = 0;
          while ((file = readdir(directoy)) != NULL)
          {
              if(file->d_name[0] != '.')
              {
                numFiles = numFiles + 1;
                fprintf(stdout, "%s", file->d_name);
                fprintf(stdout, "%s", " (");
                stat(file->d_name, &stats);
                fprintf(stdout, "%lld bytes", (long long) stats.st_size);
                fprintf(stdout, "%s\n", ")");
              }
          }

        closedir(directoy);
        return 0;
    }
    return 1;
}

int cd(char* directoryPath)
{
  int error = 0;

  //Check if .. was given
  int result = strcmp(directoryPath, "..");
  if(result == 0)
  {
    chdir("..");
  }
  else
  {
    int newDirectory = chdir(directoryPath);
    //Couldn't get into given directory
    if(newDirectory != 0)
    {
      fprintf(stderr, "%s\n", "Error: cannot cd into directory");
      error = 1;
    }
  }
  return error;
}

// if append = 0, >
// if append = 1, >>
int redirect(struct Command cmd1, struct Command cmd2, char *fileName, char *fullCommand, int append) {
	pid_t pid;

	struct Command newCmd = redirectExtraArgs(cmd1, cmd2);

	if(cmd1.args[0] == NULL) {
    		fprintf(stderr, "Error: missing command\n");
  	} else if (fileName == NULL) {
    		fprintf(stderr, "Error: no output file\n");
  	} else if(strchr(fileName, '/') != NULL) {
		fprintf(stderr, "Error: cannot open output file\n");
	} else {
    		pid = fork();
    		if (pid == 0) {
      			/* Child */
     			int fd;
			
			 if(append == 1) {
				fd = open(fileName,  O_WRONLY | O_CREAT | O_APPEND,  0644);
			} else {
				fd = open(fileName,  O_WRONLY | O_CREAT | O_TRUNC,  0644);
			} 

      			dup2(fd, STDOUT_FILENO);
      			close(fd);

      			execvp(newCmd.args[0], newCmd.args);
      			perror("execvp");
      			exit(1);
    		} else if (pid > 0) {
      			/* Parent */
      			int status;
      			waitpid(pid, &status, 0);
      			fprintf(stderr, "+ completed \'");
      			fprintf(stderr, "%s", fullCommand);
      			fprintf(stderr, "\' [");
      			fprintf(stderr, "%d", WEXITSTATUS(status));
      			fprintf(stderr, "%s", "]\n");
    		} else {
      			printf("fork");
      			exit(1);
    		}
  	}
	return 0;
}

void printComplete(char *fullCommand, int status)
{
  fprintf(stderr, "+ completed \'");
  fprintf(stderr, "%s", fullCommand);
  fprintf(stderr, "\' [");
  fprintf(stderr, "%d", status);
  fprintf(stderr, "%s", "]\n");
}

/*
    Cleans up commands
    Looks for ">" and "|"
    For example if echo hello world| grep hello
    determines that | is a token despite no whitespace
*/
int cleanCmd(char *cmdn)
{
  int numPipes = 0;
  int involvesPiping = 0;
  int involvesRedirect = 0;
  int involvesRedirectAppend = 0;
  int redirectBeforePipe = 0;

  for(int x =0; (unsigned)x<strlen(cmdn); x++)
  {

      if(cmdn[x] == '|')
      {
          if(redirectBeforePipe == 1)
          {
            redirectBeforePipe = 2;
          }
          involvesPiping = 1;
		      numPipes++;
      }
      if(cmdn[x] == '>')
      {
        if(redirectBeforePipe != 2)
        {
          redirectBeforePipe = 1;
        }
        involvesRedirect = 1;
      }
      if(cmdn[x] == '>' && cmdn[x+1] == '>')
      {
        involvesRedirectAppend = 1;
      }
      if(cmdn[x] == '|' || cmdn[x] =='>')
      {

                  for(int k = strlen(cmdn)+1; k > x; k--)
                    {
                        cmdn[k] = cmdn[k-1];
                    }
                    for(int k = strlen(cmdn)+2; k > x; k--)
                    {
                        cmdn[k] = cmdn[k-1];
                    }
                    cmdn[x+2] = ' ';
                    cmdn[x] = ' ';
                    x = x + 1;

      }
  }
	
  if(redirectBeforePipe == 2) {
	  return -1;
  }

  if(involvesPiping == 1 && involvesRedirectAppend ==1 && numPipes == 1) {
	return 5;
  }
  else if(involvesPiping == 1 && involvesRedirect == 1 && numPipes == 1) {
	return 4;
  }
  else if(involvesPiping == 1 && involvesRedirectAppend ==1 && numPipes == 2) {
	return 7;
  }
  else if(involvesPiping == 1 && involvesRedirect == 1 && numPipes == 2) {
	return 6;
  }
  else if(involvesPiping == 1 && involvesRedirectAppend ==1 && numPipes == 3) {
	  return 8;
  }
  else if(involvesPiping == 1 && involvesRedirect ==1 && numPipes == 3) {
	  return 9;
  }
  else if(involvesPiping == 1)
  {
    return 1;
  }
  else if(involvesRedirectAppend == 1)
  {
    return 2;
  }
  else if(involvesRedirect == 1)
  {
    return 3;
  }
  return 0;
}

/*
  Spiltting up by whitespace
    -Updated to take into account multiple whitespaces as in Specifications.
*/
int splitArgsBySpace(char *args[17], char *cmdn)
{
  char *farg = strtok(cmdn," ");
  int current = 0;
  while( farg != NULL ) {
    args[current] = farg;
    current++;
    farg = strtok(NULL, " ");
  }
  args[current] = NULL;

  return current;
}

int argsToCmd(char *args[17], int current)
{
  int numCommands = 0;
  int lastPosofPipe = 0;
  for(int u = 0; u < current; u++)
  {
    if(*args[u] == '|' || *args[u] == '>')
    {

       memcpy(cmds[numCommands].args, &args[lastPosofPipe], (u-lastPosofPipe)*sizeof(args[0]));
       numCommands++;
        lastPosofPipe = u+1;
    }

  }
  memcpy(cmds[numCommands].args, &args[lastPosofPipe], (current-lastPosofPipe)*sizeof(args[0]) );
  numCommands++;

   return numCommands;
}

int singlePipe(struct Command cmd1, struct Command cmd2, char *fullCommand) {

	if (cmd1.args[0] == NULL || cmd2.args[0] == NULL) {
		fprintf(stderr, "Error: missing command\n");
	} else {
		int fd[2];
		pipe(fd);
		if (fork() == 0) {
			/* Child */
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
			execvp(cmd1.args[0], cmd1.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
		} else {
			/* Parent */
			if (fork() == 0) {
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);
				close(fd[0]);
				execvp(cmd2.args[0], cmd2.args);
				fprintf(stderr, "Error: command not found\n");
				exit(1);
			}
		}
		close(fd[0]);
		close(fd[1]);

		int status[2];
		for (int i=0; i < 2; i++) {
			wait(&status[i]);
		}

		fprintf(stderr, "+ completed \'");
		fprintf(stderr, "%s", fullCommand);
		fprintf(stderr, "\' ");
		for (int x=0; x < 2; x++) {
			fprintf(stderr, "[");
			fprintf(stderr, "%d", WEXITSTATUS(status[x]));
			fprintf(stderr, "%s", "]");
		}
		fprintf(stderr, "\n");
	}
	return 0;
}


int doublePipe(struct Command cmd1, struct Command cmd2, struct Command cmd3, char *fullCommand) {
	if (cmd1.args[0] == NULL || cmd2.args[0] == NULL || cmd3.args[0] == NULL) {
		fprintf(stderr, "Error: missing command\n");
	} else {
		int fd[4];
		pipe(fd);
		pipe(fd + 2);

		if (fork() == 0) {
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);

			close(fd[1]);
			close(fd[2]);
			close(fd[3]);

			execvp(cmd1.args[0], cmd1.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
		} else {
			if (fork() == 0) {
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);

				close(fd[2]);
				dup2(fd[3], STDOUT_FILENO);

				close(fd[0]);
				close(fd[3]);

				execvp(cmd2.args[0], cmd2.args);
				fprintf(stderr, "Error: command not found\n");
				exit(1);
			} else {
				if (fork() == 0) {
					close(fd[3]);
					dup2(fd[2], STDIN_FILENO);

					close(fd[0]);
					close(fd[1]);
					close(fd[2]);
					execvp(cmd3.args[0], cmd3.args);
					fprintf(stderr, "Error: command not found\n");
					exit(1);
				}
			}
		}

		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
		close(fd[3]);

		int status[3];
		for (int i=0; i < 3; i++) {
			wait(&status[i]);
		}
		fprintf(stderr, "+ completed \'");
		fprintf(stderr, "%s", fullCommand);
		fprintf(stderr, "\' ");
		for (int x=0; x < 3; x++) {
			fprintf(stderr, "[");
			fprintf(stderr, "%d", WEXITSTATUS(status[x]));
			fprintf(stderr, "%s", "]");
		}
		fprintf(stderr, "\n");
	}
	return 0;
}

int triplePipe(struct Command cmd1, struct Command cmd2, struct Command cmd3, struct Command cmd4, char *fullCommand) {
	int fd[6];
	pipe(fd);
	pipe(fd+2);
	pipe(fd+4);

	if (fork() == 0) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);

		close(fd[1]);
		close(fd[2]);
		close(fd[3]);
		close(fd[4]);
		close(fd[5]);

		execvp(cmd1.args[0], cmd1.args);
		fprintf(stderr, "Error: command not found\n");
		exit(1);
	} else {
		if (fork() == 0) {
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);

			close(fd[2]);
			dup2(fd[3], STDOUT_FILENO);

			close(fd[0]);
			close(fd[3]);
			close(fd[4]);
			close(fd[5]);

			execvp(cmd2.args[0], cmd2.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
		} else {
			if (fork() == 0) {
				close(fd[3]);
				dup2(fd[2], STDIN_FILENO);

				close(fd[4]);
				dup2(fd[5], STDOUT_FILENO);

				close(fd[0]);
				close(fd[1]);
				close(fd[2]);
				close(fd[5]);

				execvp(cmd3.args[0], cmd3.args);
				fprintf(stderr, "Error: command not found\n");
				exit(1);
			} else {
				if (fork() == 0) {
					close(fd[5]);
					dup2(fd[4], STDIN_FILENO);

					close(fd[0]);
					close(fd[1]);
					close(fd[2]);
					close(fd[3]);
					close(fd[4]);
					execvp(cmd4.args[0], cmd4.args);
					fprintf(stderr, "Error: command not found\n");
					exit(1);
				}
			}
		}
	}
	close(fd[0]);
	close(fd[1]);
	close(fd[2]);
	close(fd[3]);
	close(fd[4]);
	close(fd[5]);

	int status[4];
	for(int i = 0; i < 4; i++) {
		wait(&status[i]);
	}
	fprintf(stderr, "+ completed \'");
	fprintf(stderr, "%s", fullCommand);
	fprintf(stderr, "\' ");
	for (int x=0; x < 4; x++) {
		fprintf(stderr, "[");
		fprintf(stderr, "%d", WEXITSTATUS(status[x]));
		fprintf(stderr, "%s", "]");
	}
	fprintf(stderr, "\n");

	return 0;
}

int singlePipeRedirect(struct Command cmd1, struct Command cmd2, struct Command cmd3, char *fullCommand, int append) {

	if (cmd1.args[0] == NULL || cmd2.args[0] == NULL) {
		fprintf(stderr, "Error: missing command\n");
	} else if (cmd3.args[0] == NULL) {
		fprintf(stderr, "Error: no output file\n");
	} else if(strchr(cmd3.args[0], '/') != NULL) {
		fprintf(stderr, "Error: cannot open output file\n");
	} else {
		int fd[2];
		pipe(fd);
		if (fork() == 0) {
			/* Child */
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
			execvp(cmd1.args[0], cmd1.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
		} else {
			/* Parent */
			if (fork() == 0) {
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);
				close(fd[0]);

				int newFd;
				if(append == 1) {
					newFd = open(cmd3.args[0], O_WRONLY | O_CREAT | O_APPEND,  0644);
				} else {
					newFd = open(cmd3.args[0], O_WRONLY | O_CREAT | O_TRUNC,  0644);
				}

      			dup2(newFd, STDOUT_FILENO);
      			close(newFd);

				struct Command newCmd = redirectExtraArgs(cmd2, cmd3);
				execvp(newCmd.args[0], newCmd.args);
				fprintf(stderr, "Error: command not found\n");
				exit(1);
			}
		}
		close(fd[0]);
		close(fd[1]);

		int status[2];
		for (int i=0; i < 2; i++) {
			wait(&status[i]);
		}

 		fprintf(stderr, "+ completed \'");
		fprintf(stderr, "%s", fullCommand);
		fprintf(stderr, "\' ");
		for (int x=0; x < 2; x++) {
			fprintf(stderr, "[");
			fprintf(stderr, "%d", WEXITSTATUS(status[x]));
			fprintf(stderr, "%s", "]");
		}
		fprintf(stderr, "\n");
	}
	return 0;
}

int doublePipeRedirect(struct Command cmd1, struct Command cmd2, struct Command cmd3, struct Command cmd4, char *fullCommand, int append) {

	if (cmd1.args[0] == NULL || cmd2.args[0] == NULL || cmd3.args[0] == NULL){
		fprintf(stderr, "Error: missing command\n");
	} else if (cmd4.args[0] == NULL) {
		fprintf(stderr, "Error: no output file\n");
	} else if(strchr(cmd4.args[0], '/') != NULL) {
		fprintf(stderr, "Error: cannot open output file\n");
	}  else {
		int fd[4];
		pipe(fd);
		pipe(fd + 2);

		if (fork() == 0) {
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);

			close(fd[1]);
			close(fd[2]);
			close(fd[3]);

			execvp(cmd1.args[0], cmd1.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
		} else {
			if (fork() == 0) {
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);

			close(fd[2]);
			dup2(fd[3], STDOUT_FILENO);

			close(fd[0]);
			close(fd[3]);

			execvp(cmd2.args[0], cmd2.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
			} else {
				if (fork() == 0) {
					close(fd[3]);
					dup2(fd[2], STDIN_FILENO);

					close(fd[0]);
					close(fd[1]);
					close(fd[2]);
					int newFd;
					if(append == 1) {
						newFd = open(cmd4.args[0], O_WRONLY | O_CREAT | O_APPEND,  0644);
					} else {
						newFd = open(cmd4.args[0], O_WRONLY | O_CREAT | O_TRUNC,  0644);
					}

					dup2(newFd, STDOUT_FILENO);
					close(newFd);

					struct Command newCmd = redirectExtraArgs(cmd3, cmd4);

					execvp(newCmd.args[0], newCmd.args);
					fprintf(stderr, "Error: command not found\n");
					exit(1);
				}
			}
		}

		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
		close(fd[3]);

		int status[3];
		for (int i=0; i < 3; i++) {
			wait(&status[i]);
		}
		fprintf(stderr, "+ completed \'");
		fprintf(stderr, "%s", fullCommand);
		fprintf(stderr, "\' ");
		for (int x=0; x < 3; x++) {
			fprintf(stderr, "[");
			fprintf(stderr, "%d", WEXITSTATUS(status[x]));
			fprintf(stderr, "%s", "]");
		}
		fprintf(stderr, "\n");
	}

	return 0;
}

int triplePipeRedirect(struct Command cmd1, struct Command cmd2, struct Command cmd3, struct Command cmd4, struct Command cmd5, char *fullCommand, int append) {
	
	if (cmd1.args[0] == NULL || cmd2.args[0] == NULL || cmd3.args[0] == NULL || cmd4.args[0] == NULL){
		fprintf(stderr, "Error: missing command\n");
	} else if (cmd5.args[0] == NULL) {
		fprintf(stderr, "Error: no output file\n");
	} else if(strchr(cmd5.args[0], '/') != NULL) {
		fprintf(stderr, "Error: cannot open output file\n");
	}  else {
		int fd[6];
		pipe(fd);
		pipe(fd+2);
		pipe(fd+4);

		if (fork() == 0) {
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);

			close(fd[1]);
			close(fd[2]);
			close(fd[3]);
			close(fd[4]);
			close(fd[5]);

			execvp(cmd1.args[0], cmd1.args);
			fprintf(stderr, "Error: command not found\n");
			exit(1);
		} else {
			if (fork() == 0) {
				close(fd[1]);
				dup2(fd[0], STDIN_FILENO);

				close(fd[2]);
				dup2(fd[3], STDOUT_FILENO);

				close(fd[0]);
				close(fd[3]);
				close(fd[4]);
				close(fd[5]);

				execvp(cmd2.args[0], cmd2.args);
				fprintf(stderr, "Error: command not found\n");
				exit(1);
			} else {
				if (fork() == 0) {
					close(fd[3]);
					dup2(fd[2], STDIN_FILENO);

					close(fd[4]);
					dup2(fd[5], STDOUT_FILENO);

					close(fd[0]);
					close(fd[1]);
					close(fd[2]);
					close(fd[5]);

					execvp(cmd3.args[0], cmd3.args);
					fprintf(stderr, "Error: command not found\n");
					exit(1);
				} else {
					if (fork() == 0) {
						close(fd[5]);
						dup2(fd[4], STDIN_FILENO);

						close(fd[0]);
						close(fd[1]);
						close(fd[2]);
						close(fd[3]);
						close(fd[4]);

						int newFd;
						if(append == 1) {
							newFd = open(cmd5.args[0], O_WRONLY | O_CREAT | O_APPEND,  0644);							
						} else {
							newFd = open(cmd5.args[0], O_WRONLY | O_CREAT | O_TRUNC,  0644);							
						}
						
						dup2(newFd, STDOUT_FILENO);
						close(newFd);
						
						struct Command newCmd = redirectExtraArgs(cmd4, cmd5);

						execvp(newCmd.args[0], newCmd.args);
						fprintf(stderr, "Error: command not found\n");
						exit(1);
					}
				}
			}
		}
		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
		close(fd[3]);
		close(fd[4]);
		close(fd[5]);

		int status[4];
		for(int i = 0; i < 4; i++) {
			wait(&status[i]);
		}
		fprintf(stderr, "+ completed \'");
		fprintf(stderr, "%s", fullCommand);
		fprintf(stderr, "\' ");
		for (int x=0; x < 4; x++) {
			fprintf(stderr, "[");
			fprintf(stderr, "%d", WEXITSTATUS(status[x]));
			fprintf(stderr, "%s", "]");
		}
		fprintf(stderr, "\n");
	}
	return 0;
}


int runSingleCommand(struct Command c, char *fullCommand)
{
      /*
        Replace system() with fork, wait, exec
      */
      pid_t pid; 
      pid = fork();
      if(pid == 0)
      {
        //CHILD
        execvp(c.args[0], c.args);
        printf("Error: command not found\n");
        exit(1);
      }
      else if(pid > 0)
      {
         //PARENT
         int status;
         waitpid(pid, &status, 0);
         /*
            Add seperate function for this LATER
              - rough works for now
              - messy way, combine to 1 line.
         */
         fprintf(stderr, "+ completed \'");
         fprintf(stderr, "%s", fullCommand);
         fprintf(stderr, "\' [");
         fprintf(stderr, "%d", WEXITSTATUS(status));
         fprintf(stderr, "%s", "]\n");

      }
      else
      {
        perror("fork");
        exit(1);
      }

      return 0;
}

int main(void)
{

        char cmd[CMDLINE_MAX];

        while (1) {
                memset(cmds, 0, sizeof(cmds));
                char *nl;              
		
                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                  /*
                    fgets(char *str, int n, FILE *stream)
                      Read a line from given stream - stores it into string str - stops when n - 1 chars read
                  */
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

                //KEEPS FULL CMD
                char full[CMDLINE_MAX];
                strcpy(full, cmd);


                int builtInUsed = 0;
		if(strlen(cmd) == 0)
                {
                  builtInUsed = 1;
                }
                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        printComplete(full, 0);
                        break;
                }


                //PWD command, referenced from slide 33 (syscalls)
                if (!strcmp(cmd, "pwd")) {
                        char cwd[PATH_MAX];
                        printf("%s\n", getcwd(cwd, sizeof(cwd)));
                        printComplete(full, 0);
                        builtInUsed = 1;
                }

                /*
                  From Project1 Specifications
                    "A Program has a maximum of 16 arguments"
                    +1 for null
                */
                char *arguments[17];

                /*
                   Two boolean varibles to see if piping or redirect is going to be used.
                   = 1 if they are going to be used.
                */

                int involvesPiping = 0;
                int involvesRedirect = 0;
                int involvesRedirectAppend = 0;
				int involvesSinglePipeRedirect = 0;
				int involvesSinglePipeAppend = 0;
				int involvesDoublePipeRedirect = 0;
				int involvesDoublePipeAppend = 0;
				int involvesTriplePipeRedirect = 0;
				int involvesTriplePipeAppend = 0;

                int cmdSearch = cleanCmd(cmd);
                if(cmdSearch == 1)
                {
                  involvesPiping = 1;
                }
                else if(cmdSearch == 2)
                {
                  involvesRedirectAppend = 1;
                }
                else if(cmdSearch == 3)
                {
                  involvesRedirect = 1;
                }
				else if(cmdSearch == 4)
				{
				involvesSinglePipeRedirect = 1;
				}
				else if(cmdSearch == 5)
				{
				involvesSinglePipeAppend = 1;
				}
				else if(cmdSearch == 6)
				{
				involvesDoublePipeRedirect = 1;
				}
				else if(cmdSearch == 7)
				{
				involvesDoublePipeAppend = 1;
				}
				else if (cmdSearch == 8)
				{
					involvesTriplePipeAppend = 1;
				}
				else if (cmdSearch == 9)
				{
					involvesTriplePipeRedirect = 1;
				}	

				if(cmdSearch == -1) {
					fprintf(stderr, "%s\n", "Error: mislocated output redirection");					
				}


                /*
                  Spiltting up by whitespace
                    -Updated to take into account multiple whitespaces as in Specifications.
                */
                int numArguments = 0;
                numArguments = splitArgsBySpace(arguments, cmd);
                if(numArguments > 16)
                {
                  fprintf(stderr, "%s\n", "Error: too many process arguments");
                  builtInUsed = 1;
                }

                //Split up arguments (tokens) to struct of commands
                int numCommands = argsToCmd(arguments, numArguments);





               if (!strcmp(cmd, "sls")) {
                    int returnStatus = sls();
                    printComplete(full, returnStatus);
                    builtInUsed = 1;
               }


                if (!strcmp(cmd, "cd")) {
                        int returnStatus = cd(cmds[0].args[1]);
                        printComplete(full, returnStatus);
                        builtInUsed = 1;
                }

				if(involvesSinglePipeRedirect) {
					singlePipeRedirect(cmds[0], cmds[1], cmds[2], full, 0);
				}
				else if(involvesRedirectAppend)
				{
					redirect(cmds[0], cmds[2], cmds[2].args[0], full, 1);
				}
				else if(involvesRedirect)
				{
					redirect(cmds[0], cmds[1], cmds[1].args[0], full, 0);
					builtInUsed = 1;
				}
				else if(involvesDoublePipeRedirect) {
					doublePipeRedirect(cmds[0], cmds[1], cmds[2], cmds[3], full, 0);
				} 
				else if(involvesTriplePipeRedirect) {
					triplePipeRedirect(cmds[0], cmds[1], cmds[2], cmds[3], cmds[4], full, 0);
				}
				else if(involvesSinglePipeAppend)
				{
					singlePipeRedirect(cmds[0], cmds[1], cmds[3], full, 1);
				} 
				else if(involvesDoublePipeAppend)
				{
					doublePipeRedirect(cmds[0], cmds[1], cmds[2], cmds[4], full, 1);
				}
				else if(involvesTriplePipeAppend) {
					triplePipeRedirect(cmds[0], cmds[1], cmds[2], cmds[3], cmds[5], full, 1);
				}

				if(involvesPiping == 1)
				{
					if(numCommands == 2)
					{
						singlePipe(cmds[0], cmds[1], full);
					}
					else if(numCommands == 3)
					{
						doublePipe(cmds[0], cmds[1], cmds[2], full);
					}
					else if (numCommands == 4)
					{
						triplePipe(cmds[0], cmds[1], cmds[2], cmds[3], full);
					}
					builtInUsed = 1;
				}





                // A built in command was not used.
                if(builtInUsed == 0)
                {
                    if(numCommands == 1)
                    {
                      runSingleCommand(cmds[0], full);
                    }
                }




        }

        return EXIT_SUCCESS;
}
