#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 10

int getcmd(char *buf, int nbuf) {
    write(2, ">>> ", 4);   
    memset(buf, 0, nbuf);
    int n = 0;
    while (n + 1 < nbuf) {
        int r = read(0, buf + n, 1);
        if (r < 1) return -1;
        if (buf[n] == '\n') break;
        n++;
    }
    buf[n] = 0;
    return 0;
}

void clean_and_tokenize(char *buf) {
    char tmp[200];
    int k = 0;
    for (int i = 0; buf[i]; i++) {
        if (buf[i] == '|' || buf[i] == '<' || buf[i] == '>' || buf[i] == ';') {
            tmp[k++] = ' ';
            tmp[k++] = buf[i];
            tmp[k++] = ' ';
        } else {
            tmp[k++] = buf[i];
        }
    }
    tmp[k] = 0;
    strcpy(buf, tmp);
}

int parse(char *buf, char **argv) {
    int argc = 0;
    while (*buf) {
        while (*buf == ' ' || *buf == '\t')
            buf++;
        if (*buf == 0) break;
        argv[argc++] = buf;
        if (argc >= MAXARGS) break;

        while (*buf && *buf != ' ' && *buf != '\t')
            buf++;
        if (*buf) {
            *buf = 0;
            buf++;
        }
    }
    argv[argc] = 0;
    return argc;
}

__attribute__((noreturn))
void run_command(char *buf, int nbuf, int *pcp) {
    clean_and_tokenize(buf);

    char *argv[MAXARGS];
    parse(buf, argv);
    if (argv[0] == 0) exit(0);

    // Check for sequence operator ';'
    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], ";") == 0) {
            argv[i] = 0;
            if (fork() == 0) {
                exec(argv[0], argv);
                exit(0);
            }
            wait(0);
            // Run the rest after ';'
            run_command(argv[i+1], nbuf, pcp);
        }
    }

    if (strcmp(argv[0], "cd") == 0) {
        char path[100];
        if (argv[1])
            strcpy(path, argv[1]);
        else
            strcpy(path, "/");
        write(pcp[1], path, strlen(path)+1);
        exit(2);
    }

    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], "|") == 0) {
            argv[i] = 0;
            int fd[2];
            pipe(fd);

            if (fork() == 0) {
                close(1);
                dup(fd[1]);
                close(fd[0]);
                close(fd[1]);
                exec(argv[0], argv);
                exit(0);
            }

            close(fd[1]);
            close(0);
            dup(fd[0]);
            close(fd[0]);
            exec(argv[i+1], &argv[i+1]);
            exit(0);
        }
    }

    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], "<") == 0) {
            int fd = open(argv[i+1], O_RDONLY);
            close(0);
            dup(fd);
            close(fd);
            argv[i] = 0;
        }
        if (strcmp(argv[i], ">") == 0) {
            int fd = open(argv[i+1], O_CREATE | O_WRONLY);
            close(1);
            dup(fd);
            close(fd);
            argv[i] = 0;
        }
    }

    exec(argv[0], argv);
    fprintf(2, "exec %s failed\n", argv[0]);
    exit(0);
}

int main() {
    static char buf[200];
    int pcp[2];
    pipe(pcp);

    while (getcmd(buf, sizeof(buf)) >= 0) {
        if (fork() == 0)
            run_command(buf, sizeof(buf), pcp);

        int st;
        wait(&st);

        if (st == 512) { 
            char path[100];
            read(pcp[0], path, sizeof(path));
            if (chdir(path) < 0)
                fprintf(2, "cannot cd %s\n", path);
        }
    }
    exit(0);
}
