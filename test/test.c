#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void){
  char another[16] = "123456789012345";
  char str[40] = "1234567890123456789" \ "123456789012345678";

  strncpy(another, str, strlen(another));
  another[strlen(another]
  return 0;
}
