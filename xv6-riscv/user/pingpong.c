#include "user/user.h"
#include "kernel/types.h"

int main(){
    int p2c[2];
    int c2p[2];

    pipe(p2c);
    pipe(c2p);

    char our_byte = 'P';

    int pid = fork();
    if(pid>0){
        if (write(p2c[1], &our_byte, 1) != 1) {
            printf("parent: write to pipe failed\n");
            exit(1);
        }
        if (read(c2p[0], &our_byte, 1) != 1) {
            printf("parent: read from pipe failed\n");
            exit(1);
        }
        printf("pingpong: received pong '%c'\n", our_byte);

        close(p2c[1]);
        close(c2p[0]);
        exit(0);
    }else if (pid==0){
        if (read (p2c[0], &our_byte, 1) != 1) {
            printf("child: read from pipe failed\n");
            exit(1);
        }
        printf("pingpong: received ping '%c'\n", our_byte);
        our_byte = 'R';

        if (write(c2p[1], &our_byte, 1) != 1) {
            printf("child: write to pipe failed\n");
            exit(1);
        }
        close(p2c[0]);
        close(c2p[1]);
        exit(0);
     }else{
        printf("pingpong: fork failed\n");
        exit(1);
     }
     return 0;
}