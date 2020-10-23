sshell: sshell.o
        gcc -02 -Wall -Wextra -Werror -o sshell sshell.o
        
sshell.o: sshell.c
        gcc -02 -Wall -Wextra -Werror -c -o sshell.o sshell.c
        
.PHONY: clean

clean:
        -rm -f sshell sshell.o
