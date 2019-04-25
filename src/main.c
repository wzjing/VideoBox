#include "utils/log.h"
#include "demux_decode.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "demux") == 0) {
      demux_decode(argv[1], argv[2], argv[3]);
      return 0;
    }
  }

  fprintf(stderr, "invalid command: ");
  for (int j = 0; j < argc; ++j) {
    printf("%s ", argv[j]);
  }
  printf("\n");
  return -1;
}