#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

int main(){
  char *a = malloc(4);
  printf("%s\n", "Im gonna make a pro gamer move!");
  strcpy(a, "You Suck");
  printf("%s\n", "OHHHHHHHHHHHHH");
  return 0;
}
