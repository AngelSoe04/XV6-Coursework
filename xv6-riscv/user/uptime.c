#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int ticks;
    ticks = uptime();
    printf("Uptime: %d\n", ticks);
    exit(0);
}