//
// Created by Infinity on 2019-04-21.
//

#include "scale.h"
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include "../utils/log.h"

int yuv2rgb(struct SwsContext *sws_ctx,
            uint8_t *src_data[4], int src_linesize[4], int src_w, int src_h,
            uint8_t *dst_data[4], int dst_linesize[4], int dst_w, int dst_h) {

  if (!sws_ctx) {
    fprintf(stderr, "SwsContext is no available\n");
    goto end;
  }

  int ret;
  if ((ret = av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, AV_PIX_FMT_RGB24, 1)) < 0) {
    fprintf(stderr, "Could not allocate dst image memory\n");
    goto end;
  }

  sws_scale(sws_ctx, (const uint8_t *const *) src_data, src_linesize, 0, src_h, dst_data, dst_linesize);

  end:
  return ret < 0;
}

void convert(AVFrame *src, enum AVPixelFormat dst_fmt, int dst_width, int dst_height, struct SwsContext *sws_ctx,
             uint8_t *dst_data[4], int dst_linesize[4]) {

  int ret = 0;

  // set variables

  if (dst_width == 0) dst_width = src->width;
  if (dst_height == 0) dst_height = src->height;

  if (!dst_linesize) {
    dst_linesize = malloc(sizeof(int) * 4);
  }

  if (!dst_data) {
    dst_data = malloc(sizeof(uint8_t *) * 4);
    ret = av_image_alloc(dst_data, dst_linesize, dst_width, dst_height, dst_fmt, 1);
    if (ret < 0) {
      LOGE("scale: Unable to allocate an temp data\n");
      goto end;
    }
  }


  if (!sws_ctx) {
    sws_ctx = sws_getContext(src->width, src->height, src->format,
                             dst_width, dst_height, dst_fmt,
                             SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
      LOGE("scale: Unable to allocate and temp SwsContext\n");
      goto end;
    }
  }
  // convert format
  sws_scale(sws_ctx, (const uint8_t *const *) src->data, src->linesize, 0, dst_height, dst_data,
            dst_linesize);

  end:
  memset(dst_linesize, 0, 4 * sizeof(int));
}
