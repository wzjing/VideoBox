//
// Created by Infinity on 2019-04-21.
//

#include "scale.h"
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

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
