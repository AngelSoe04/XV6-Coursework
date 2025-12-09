#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXARGS 10
#define MAXBUF 100

// Part 1: Prompt and read command line
int getcmd(char *buf, int nbuf) {
    int i = 0;
    char c;

    // Print prompt to stderr
    write(2, ">>> ", 4);

    while (i < nbuf) {
        if (read(0, &c, 1) < 1) // EOF or error
            return -1;
        if (c == '\n') { // end of input
            buf[i] = 0;
            return i;
        }
        buf[i++] = c;
    }
    buf[nbuf - 1] = 0;
    return nbuf - 1;
}

// Helper: split buf into arguments, return number of args
int parse_arguments(char *buf, char **args) {
    int argc = 0;
    char *p = buf;

    while (*p) {
        // skip spaces
        while (*p && (*p == ' ' || *p == '\t'))
            p++;
        if (*p == 0)
            break;

        args[argc++] = p;
        if (argc >= MAXARGS)
            break;

        // find end of word
        while (*p && *p != ' ' && *p != '\t' && *p != '|' && *p != '>' && *p != '<' && *p != ';')
            p++;

        if (*p) {
            *p = 0; // terminate the word
            p++;
        }
    }
    args[argc] = 0;
    return argc;
}

// Recursive command execution
__attribute__((noreturn))
void run_command(char *buf, int nbuf, int *pcp) {
    char *args[MAXARGS];
    int argc = 0;

    // Flags
    int pipe_cmd = 0;
    int sequence_cmd = 0;
    int redir_left = 0, redir_right = 0;
    char *file_left = 0, *file_right = 0;

    int i = 0;

    // Parse the buffer for special characters
    for (i = 0; buf[i]; i++) {
        if (buf[i] == '|') {
            pipe_cmd = 1;
            buf[i] = 0;
        }
        if (buf[i] == ';') {
            sequence_cmd = 1;
            buf[i] = 0;
        }
        if (buf[i] == '<') {
            redir_left = 1;
            buf[i++] = 0;
            while (buf[i] == ' ') i++;
            file_left = &buf[i];
            while (buf[i] && buf[i] != ' ' && buf[i] != '\t') i++;
            if (buf[i]) buf[i++] = 0;
            i--;
        }
        if (buf[i] == '>') {
            redir_right = 1;
            buf[i++] = 0;
            while (buf[i] == ' ') i++;
            file_right = &buf[i];
            while (buf[i] && buf[i] != ' ' && buf[i] != '\t') i++;
            if (buf[i]) buf[i++] = 0;
            i--;
        }
    }

    // Parse command arguments
    argc = parse_arguments(buf, args);
    if (argc == 0) exit(0);

    // Sequence command ';'
    if (sequence_cmd) {
        if (fork() == 0)
            run_command(buf, nbuf, pcp); // execute first command
        wait(0);
        char *rest = buf + (buf + nbuf - buf); // safe pointer arithmetic
        run_command(rest, nbuf, pcp); // execute the rest
    }

    // Input redirection
    if (redir_left) {
        int fd = open(file_left, O_RDONLY);
        if (fd < 0) {
            fprintf(2, "cannot open %s\n", file_left);
            exit(1);
        }
        close(0);
        dup(fd);
        close(fd);
    }

    // Output redirection
    if (redir_right) {
        int fd = open(file_right, O_CREATE | O_WRONLY);
        if (fd < 0) {
            fprintf(2, "cannot open %s\n", file_right);
            exit(1);
        }
        close(1);
        dup(fd);
        close(fd);
    }

    // Handle cd command in child
    if (strcmp(args[0], "cd") == 0) {
        // Write path to parent through pipe
        write(pcp[1], args[1], strlen(args[1]) + 1);
        exit(2);
    }

    // Pipe command
    if (pipe_cmd) {
        int fd[2];
        pipe(fd);

        if (fork() == 0) {
            close(fd[0]); // close read
            close(1);
            dup(fd[1]); // stdout -> pipe write
            close(fd[1]);
            exec(args[0], args);
            fprintf(2, "exec failed\n");
            exit(1);
        }

        close(fd[1]); // close write
        close(0);
        dup(fd[0]); // stdin -> pipe read
        close(fd[0]);
        // Execute next command recursively
        char *next = buf + strlen(args[0]) + 1;
        run_command(next, nbuf, pcp);
    }

    // Simple command execution
    exec(args[0], args);
    fprintf(2, "exec %s failed\n", args[0]);
    exit(1);
}

int main(void) {
    static char buf[MAXBUF];
    int pcp[2];
    pipe(pcp);

    while (getcmd(buf, sizeof(buf)) >= 0) {
        if (fork() == 0)
            run_command(buf, sizeof(buf), pcp);

        // Parent handles cd
        int status;
        wait(&status);
        if (status == 512) { // exit(2) returned 2 -> cd command
            char path[100];
            read(pcp[0], path, sizeof(path));
            if (chdir(path) < 0)
                fprintf(2, "cannot cd %s\n", path);
        }
    }
    exit(0);
}
