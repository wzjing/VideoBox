//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_DECODE_H
#define VIDEOBOX_DECODE_H

#include "../utils/log.h"
#include <functional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

typedef const std::function<void(AVFrame *frame)> &DECODE_CALLBACK;

int decode_packet(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, DECODE_CALLBACK callback);

int decode_h264(const char *input_file, DECODE_CALLBACK callback);

#endif //VIDEOBOX_DECODE_H
