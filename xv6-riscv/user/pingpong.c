#include "user/user.h"
#include "kernel/types.h"

int main(){
    int p2c[2];
    int c2p[2];
    char our_byte = 'P';

    pipe(p2c);
    pipe(c2p);


    int pid = fork();
    if(pid>0){
       close (p2c[0]);
       close (c2p[1]);
       write (p2c[1], &our_byte, 1);
       pid = wait ((int *)0);
       int my_pid = getpid();
       char received_byte;
       read (c2p[0], &received_byte, 1);
       printf(" %d Received pong, %c \n", my_pid, received_byte);
    } else if (pid == 0){
        close (p2c[1]);
        close (c2p[0]);
        char the_byte;
        read (p2c[0], &the_byte, 1);
        int my_pid = getpid();
        printf(" %d Received ping, %c \n", my_pid, the_byte);
        if (the_byte == 'P'){
            the_byte = 'R';
            write (c2p[1], &the_byte, 1);
            exit(0);
        }
        exit(0);
    }
}