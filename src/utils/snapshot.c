//
// Created by android1 on 2019/4/22.
//

#include "snapshot.h"
#include <stdio.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include "log.h"

int save_pgm(uint8_t *buf, int wrap, int width, int height, const char *filename) {
  FILE *f;
  int i;

  f = fopen(filename, "w");
  fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);
  for (i = 0; i < height; i++)
    fwrite(buf + i * wrap, 1, width, f);
  fclose(f);
  return 0;
}

int save_ppm(uint8_t *buf, int wrap, int width, int height, const char *filename) {
  LOGD("save ppm\n");
  FILE *f;
  LOGD("opening file: %s\n", filename);
  f = fopen(filename, "w");
  LOGD("opening file: %s\n", filename);
  fprintf(f, "P6\n%d %d\n%d\n", width, height, 255);
  for (int i = 0; i < height; i++) {
    fwrite(buf + i * wrap, 3, width, f);
  }
  LOGD("write done\n");
  fclose(f);
  LOGD("closed\n");
  return 0;
}

int save_yuv(uint8_t **buf, const int *wrap, int width, int height, const char *filename) {
  FILE *f;
  f = fopen(filename, "w");

  for (int i = 0; i < height; i++) {
    fwrite(buf[0] + wrap[0] * i, 1, width, f);
  }

  for (int i = 0; i < height / 2; i++) {
    fwrite(buf[1] + wrap[1] * i, 1, width / 2, f);
  }

  for (int i = 0; i < height; i++) {
    fwrite(buf[2] + wrap[2] * i, 1, width / 2, f);
  }

  fclose(f);
  return 0;
}

int save_av_frame(AVFrame *frame, char *filename) {
  switch (frame->format) {
    case AV_PIX_FMT_RGB24:
      save_ppm(frame->data[0], frame->linesize[0], frame->width, frame->height, filename);
      LOGD("snapshot: saved snapshot %s\n", filename);
      return 0;
    case AV_PIX_FMT_YUV420P:
      save_yuv(frame->data, frame->linesize, frame->width, frame->height, filename);
      return 0;
    default:
      LOGD("snapshot: UnSupport pix format %s\n", av_get_pix_fmt_name(frame->format));
      return 0;
  }
}
