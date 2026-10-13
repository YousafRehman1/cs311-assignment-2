#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// sixfive: print every number in the input that is a multiple of 5 or 6.
//
// A number is a run of decimal digits bounded on both sides by a
// separator character (or the start/end of the file).  So the "6" in
// "xv6" is not a number, but the "6" in "/6," is.
//
// Usage: sixfive [file ...]   (reads standard input if no files given)

static const char *separators = " -\r\t\n./,";

// Print n if it is a multiple of 5 or 6.
static void
check(uint n)
{
  if (n % 5 == 0 || n % 6 == 0)
    printf("%d\n", n);
}

// Scan one file descriptor, one character at a time.
static void
sixfive(int fd, char *name)
{
  char buf[512];
  int n, i;
  uint value = 0;   // value of the digit run being scanned
  int indigits = 0; // currently inside a run of digits?
  int valid = 0;    // did the current run start right after a separator?
  int aftersep = 1; // was the previous char a separator? (start of file counts)

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      char c = buf[i];
      if (c >= '0' && c <= '9') {
        if (!indigits) {
          indigits = 1;
          valid = aftersep;
          value = 0;
        }
        value = value * 10 + (c - '0');
        aftersep = 0;
      } else if (c != 0 && strchr(separators, c)) {
        // A separator ends the run; it counts only if it also started
        // after a separator.
        if (indigits && valid)
          check(value);
        indigits = 0;
        aftersep = 1;
      } else {
        // Any other character means the adjacent digits are not a number.
        indigits = 0;
        aftersep = 0;
      }
    }
  }
  if (n < 0)
    fprintf(2, "sixfive: read error on %s\n", name);

  // End of file also counts as a separator.
  if (indigits && valid)
    check(value);
}

int
main(int argc, char *argv[])
{
  int i, fd;

  if (argc < 2) {
    sixfive(0, "stdin");
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      continue;
    }
    sixfive(fd, argv[i]);
    close(fd);
  }
  exit(0);
}
