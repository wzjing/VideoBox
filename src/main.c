#include <stdio.h>
#include <string.h>
#include "muxing/demuxing.h"
#include "codec/decode.h"

int main(int argc, char *argv[]) {

    if (strcmp(argv[1], "demux") == 0 && argc == 5) {
        demuxing(argv[2], argv[3], argv[4]);
        return 0;
    } else if (strcmp(argv[1], "decode") == 0 && argc == 4) {
      decode_h264(argv[2], argv[3]);
        return 0;
    } else {
        fprintf(stderr, "invalid command\n");
        return -1;
    }
}