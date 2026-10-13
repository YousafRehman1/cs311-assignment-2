#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

// Print the first len bytes of data, interpreted according to fmt:
//   i  4-byte int, decimal      p  8-byte integer, hex
//   h  2-byte int, decimal      c  1 byte, as an ASCII character
//   s  8-byte pointer to a C string; print the string
//   S  the rest of data is a NUL-terminated string; print it
// Never reads beyond the len valid bytes of data.
void
memdump(char *fmt, char *data, int len)
{
  int off = 0; // offset of the next unread byte in data

  for (char *f = fmt; *f; f++) {
    int need;  // bytes this format character consumes
    switch (*f) {
    case 'i': need = sizeof(int); break;
    case 'p': need = sizeof(uint64); break;
    case 'h': need = sizeof(short); break;
    case 'c': need = sizeof(char); break;
    case 's': need = sizeof(char *); break;
    case 'S': need = 0; break; // consumes whatever is left
    default:
      printf("memdump: unknown format '%c'\n", *f);
      return;
    }

    if (len - off < need) {
      printf("memdump: not enough data for '%c'\n", *f);
      return;
    }

    // Copy values out with memcpy, since data may not be suitably
    // aligned for a direct load of a multi-byte value.
    switch (*f) {
    case 'i': {
      int v;
      memcpy(&v, data + off, need);
      printf("%d\n", v);
      break;
    }
    case 'p': {
      uint64 v;
      memcpy(&v, data + off, need);
      printf("%lx\n", v);
      break;
    }
    case 'h': {
      short v;
      memcpy(&v, data + off, need);
      printf("%d\n", v);
      break;
    }
    case 'c':
      printf("%c\n", data[off]);
      break;
    case 's': {
      char *p;
      memcpy(&p, data + off, need);
      printf("%s\n", p ? p : "(null)");
      break;
    }
    case 'S': {
      // Stop at the NUL byte or the end of valid data, whichever is first.
      int n = 0;
      while (off + n < len && data[off + n] != '\0')
        n++;
      if (n > 0 && write(1, data + off, n) != n) {
        fprintf(2, "memdump: write failed\n");
        return;
      }
      printf("\n");
      return; // S consumes the rest of the data
    }
    }
    off += need;
  }
}
