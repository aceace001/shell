sshell : sshell.c
        gcc -Wall -Wextra -Werror -o sshell sshell.c
        
.PHONY : clean
clean :
        -rm -f sshell
