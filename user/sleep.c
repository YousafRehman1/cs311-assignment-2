#include "kernel/types.h"
#include "user/user.h"

// sleep: pause for a user-specified number of clock ticks.
//
// Usage: sleep <ticks>
int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(2, "Usage: sleep <ticks>\n");
    exit(1);
  }

  // atoi() returns 0 for non-numeric input, so "sleep z" behaves like
  // "sleep 0" and returns immediately.
  int ticks = atoi(argv[1]);
  if (ticks < 0) {
    fprintf(2, "sleep: invalid tick count %s\n", argv[1]);
    exit(1);
  }

  if (pause(ticks) < 0) {
    fprintf(2, "sleep: pause failed\n");
    exit(1);
  }

  exit(0);
}
