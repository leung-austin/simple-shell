sshell: sshell.c
	gcc -Wall -Wextra -O2 -o sshell sshell.c
clean:
	rm *.o $(objects) sshell
