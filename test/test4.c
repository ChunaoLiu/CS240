#include <stdio.h>

int main(int argc,  char *argv[])
{
  FILE *fileptr = 0;
  char ARRAY[51];

  fileptr = fopen(argv[1], "r");
  fscanf(fileptr, "%50s", ARRAY);
  printf("%s\n",ARRAY);

  char rest[100];
  fscanf(fileptr, "%s", rest);
  printf("%s\n", rest);
  return 0;
}
