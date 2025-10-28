#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

void usage() {
  fprintf(1, "usage: find path expression\n");
}

void find(char *path, char *expression) {

  char buf[512], *p, *e, *b;
  
  int fd;

  // Status of the opened path.
  struct stat st;

  struct dirent de;

  // Open the specified path for reading.
  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    exit(1);
  }

  // Get the status about the opened path.
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    exit(1);
  }

  // Check if the path is a file or a directory.
  switch (st.type) {
  case T_FILE:
    // Pointerers to the first character in the path.
    p = path;
    b = p;
    // Pointer to the end of the path (after the last char).
    e = p + strlen(path);
    // Loop through each character in the path and find the
    // last slash. Pointer b at the end points to the character
    // after the last slash (start of the file name).
    while (p < e) {
      if (*p++ == '/') {
        b = p;
      }
    }

    // Compare the file name at the end of the path to the file
    // name which we are search for. If equil, print the path.
    if (strcmp(b, expression) == 0) {
      fprintf(1, "%s\n", path);
    }
    break;
  case T_DIR:
    // Copy the path of the directory (a string) into buf.
    strcpy(buf, path);
    // Point p to the end of the path.
    p = buf + strlen(buf);
    // Write a slash to the end of the path. Move p to point to
    // the location after the slash.
    *p++ = '/';

    // Read each item from the directory under path. Append the name
    // of the read directory or the file to the path. Call find with
    // the modified path.
    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0 || strcmp(de.name, ".") == 0 || 
          strcmp(de.name, "..") == 0) { // ??
        continue;
      }
      // Copies the name of the new directory or file to the end of
      // the path (after the '/' which we added previously).
      memmove(p, de.name, DIRSIZ);
      // Terminate the path. DIRSIZ is the maximum number of characters
      // xv6 allows.
      p[DIRSIZ] = 0;
      // Call find recursively with the new path and repeat the whole
      // process.
      find(buf, expression);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  
  if (argc < 3) {
    usage();
    exit(1);
  }

  // Pass the directory name to start the search in and
  // the file name to search for.
  find(argv[1], argv[2]);
  exit(0);
}