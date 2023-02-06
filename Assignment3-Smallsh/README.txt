Author: Long To Lotto Tang

OSU CS344 - Smallsh

Descriptions:

- This program simulates the bash shell in C
- Execute 'cd', 'exit', 'status' in own code; other commands through fork() and exec()
- Allow input and output redirection
- Ignore SIGINT (default setting for foreground only mode)
- SIGTSTP : switch to foreground only mode and normal

Instructions:

1) Copy smallsh.c into your server

2) Compile using the following command in unix:

    gcc -std=c99 -o smallsh smallsh.c 

3) Type the following command to run on your server:
    
    ./smallsh
