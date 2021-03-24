#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

int main() {
  FILE *file_1 = fopen("/homes/cs240/public/homework/proj1/testing/good_music/J_M_Bach_Auf_lasst_uns_den_Herren_loben.mid", "r");
  FILE *file_2 = fopen("./my_Bach.mid", "r");
  int counter = 0;
  while (!feof(file_1)) {
    uint8_t buffer = 0;
    fread(&buffer, 1, 1, file_1);
    uint8_t buffer2 = 0;
    fread(&buffer2, 1, 1, file_2);
    printf("%x : ", buffer);
    printf("%x\n", buffer2);
    if (buffer != buffer2) {
      break;
    }
    counter += 1;
  }
  fclose(file_1);
  fclose(file_2);
}
