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

/**
 * Convert an AVFrame to another configuration
 * @param src           The source AVFrame
 * @param dst_fmt       The destination AVPixelFormat to convert to
 * @param dst_width     The destination width to convert to
 * @param dst_height    The destination height to convert to
 * @param sws_ctx       A SwsContext pointer, user must free it after done all convert work.
 * @param dst_data      A pointer to retrieve the result data
 * @param dst_linesize  A pointer to retrieve the result line-size
 *
 * @return
 */
void convert(AVFrame *src, enum AVPixelFormat dst_fmt, int dst_width, int dst_height, struct SwsContext *sws_ctx,
             uint8_t *dst_data[4], int dst_linesize[4]);

#endif //VIDEOBOX_SCALE_H
