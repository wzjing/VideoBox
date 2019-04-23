
#include "muxing/demuxing.h"
#include "codec/decode.h"
#include "utils/snapshot.h"
#include "image/scale.h"
#include "utils/log.h"


#include <stdio.h>
#include <string.h>


//const char *filename;
//struct SwsContext *sws_ctx = NULL;
//uint8_t **data;
//int *linesize = NULL;
//int i = 0;

//void saveFrame(AVFrame *frame) {
//  char buf[256];
//  LOGD("start converting..\n");
//  convert(frame, AV_PIX_FMT_RGB24, 0, 0, sws_ctx, data, linesize);
//  LOGD("format output name..\n");
//  snprintf(buf, sizeof(buf), "%s-%d%s", filename, i, ".ppm");
//  LOGD("saving file: %s\n", buf);
//  save_ppm(data[0], linesize[0], frame->width, frame->height, buf);
//  i++;
//  av_freep(data);
//}

void cb(AVFrame * frame){
  LOGD("order: %d\n", frame->coded_picture_number);
}

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "demux") == 0 && argc == 5) {
      demux(argv[2], NULL);
      return 0;
    } else if (strcmp(argv[1], "decode") == 0 && argc == 4) {
      decode_h264(argv[2], cb);
      LOGE("decode done\n");
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