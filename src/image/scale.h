//
// Created by Infinity on 2019-04-21.
//

#ifndef VIDEOBOX_SCALE_H
#define VIDEOBOX_SCALE_H

#include <libswscale/swscale.h>
#include <libavutil/frame.h>

int yuv2rgb(struct SwsContext *sws_ctx,
            uint8_t *src_data[4], int src_linesize[4], int src_w, int src_h,
            uint8_t *dst_data[4], int dst_linesize[4], int dst_w, int dst_h);

#endif //VIDEOBOX_SCALE_H
