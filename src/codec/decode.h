//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_DECODE_H
#define VIDEOBOX_DECODE_H

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

static void decode_packet(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, const char *filename);

int decode_h264(const char *input_file, const char *output_file);

#endif //VIDEOBOX_DECODE_H
