#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

// find: recursively search a directory tree for files and directories
// with a given name.
//
// Usage: find <dir> <name> [-exec cmd [args ...]]
//
// Without -exec, the path of every match is printed.  With -exec, the
// command "cmd args ... path" is run (fork/exec/wait) for every match
// instead.

static char *target;     // the name being searched for
static char **execargv;  // command (and its args) given after -exec, or 0
static int execargc;     // number of entries in execargv

// Return a pointer to the last component of path.
static char *
basename(char *path)
{
  char *p;

  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  return p + 1;
}

// Handle one match: print its path, or run the -exec command on it.
static void
found(char *path)
{
  char *argv[MAXARG];
  int i, pid;

  if (execargv == 0) {
    printf("%s\n", path);
    return;
  }

  // Build "cmd args ... path"; main() guarantees this fits in MAXARG.
  for (i = 0; i < execargc; i++)
    argv[i] = execargv[i];
  argv[i++] = path;
  argv[i] = 0;

  pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    return;
  }
  if (pid == 0) {
    exec(argv[0], argv);
    fprintf(2, "find: exec %s failed\n", argv[0]);
    exit(1);
  }
  if (wait(0) < 0)
    fprintf(2, "find: wait failed\n");
}

// Search the directory at path (recursively) for entries named target.
static void
find(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // If the starting point is not a directory, just check its own name.
  if (st.type != T_DIR) {
    if (strcmp(basename(path), target) == 0)
      found(path);
    close(fd);
    return;
  }

  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
    fprintf(2, "find: path too long: %s\n", path);
    close(fd);
    return;
  }
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;

    // Directory entry names are not necessarily NUL-terminated.
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    // Never recurse into "." or "..", or the search would loop forever.
    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
      continue;

    if (stat(buf, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }

    if (strcmp(p, target) == 0)
      found(buf);

    if (st.type == T_DIR)
      find(buf);
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    fprintf(2, "Usage: find <dir> <name> [-exec cmd [args ...]]\n");
    exit(1);
  }

  // Initialize the globals explicitly rather than relying on a zeroed
  // BSS, which a kernel that does not scrub new pages won't provide.
  target = argv[2];
  execargv = 0;
  execargc = 0;

  if (argc > 3) {
    if (strcmp(argv[3], "-exec") != 0 || argc < 5) {
      fprintf(2, "Usage: find <dir> <name> [-exec cmd [args ...]]\n");
      exit(1);
    }
    execargv = &argv[4];
    execargc = argc - 4;
    // Leave room for the matched path and the terminating 0.
    if (execargc + 2 > MAXARG) {
      fprintf(2, "find: too many arguments for -exec\n");
      exit(1);
    }
  }

  find(argv[1]);
  exit(0);
}
