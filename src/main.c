
#include "muxing/demuxing.h"
#include "codec/decode.h"
#include "utils/snapshot.h"
#include "image/scale.h"
#include "utils/log.h"

#include <stdio.h>
#include <string.h>

const char *filename;
struct SwsContext *sws_ctx = NULL;
int i = 0;

void saveFrame(AVFrame *frame) {
  if (!frame) return;
  uint8_t *data[4];
  int linesize[4];

  if (!sws_ctx) {
    sws_ctx = sws_getContext(frame->width, frame->height, frame->format,
                   frame->width, frame->height, AV_PIX_FMT_RGB24,
                   SWS_BILINEAR, NULL, NULL, NULL);
  }

  if (av_image_alloc(data, linesize, frame->width, frame->height, AV_PIX_FMT_RGB24, 1) < 0) {
    LOGE("scale: Unable to allocate an temp data\n");
    exit(1);
  }

  sws_scale(sws_ctx, (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height, data,
            linesize);

  char buf[256];
  snprintf(buf, sizeof(buf), "%s-%d%s", filename, i, ".ppm");
  save_ppm(data[0], linesize[0], frame->width, frame->height, buf);
  i++;
  av_freep(data);
}

void cb(AVFrame *frame) {
  if (frame) {
    LOGD("order: %d\n", frame->coded_picture_number);
  }
}

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "demux") == 0 && argc == 5) {
      demux(argv[2], NULL);
      return 0;
    } else if (strcmp(argv[1], "decode") == 0 && argc == 4) {
      filename = argv[3];
      decode_h264(argv[2], saveFrame);
      if (sws_ctx) sws_freeContext(sws_ctx);
      LOGD("decode done\n");
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