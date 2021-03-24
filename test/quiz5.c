#include <stdio.h>

int main() {
  char buf[8] = "Hello!";
  long my_long = 0xffffffffffffffff;
  char my_char = 'B';
  int my_int = 7;

  printf("%p\n%p\n%p\n%p\n%p\n%p\n%p\n", buf, &buf[1], &buf[2], &buf[3], &my_long, &my_char, &my_int);

  return 0;
}
