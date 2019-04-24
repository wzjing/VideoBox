//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_DECODE_H
#define VIDEOBOX_DECODE_H

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include "../utils/log.h"

typedef void (*DECODE_CALLBACK)(AVFrame* frame);

static void decode_packet(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, DECODE_CALLBACK callback);

int decode_h264(const char *input_file, DECODE_CALLBACK callback);

#endif //VIDEOBOX_DECODE_H
