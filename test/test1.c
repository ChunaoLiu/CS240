#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main() {
  FILE *file = fopen("file.txt", "w");
  uint32_t value = 0xC08000;
  fwrite(&value, sizeof(value), 1, file);
  fclose(file);
  file = fopen("file.txt", "r");
  uint8_t d;
  uint32_t out = 0x0;
  for (int i = 0; i < 3; i++) {
    fread(&d, 1, 1, file);
    uint32_t temp = d;
    printf("%x\n", d);
    out |= (temp & 127) << (7 * (i));
  }
  printf("\n%d\n", out);
  return 1;
}
