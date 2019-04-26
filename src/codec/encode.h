//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_ENCODE_H
#define VIDEOBOX_ENCODE_H

#include <functional>
#include "../utils/log.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

typedef const std::function<void(AVPacket *pkt)> &ENCODE_CALLBACK;

void encode_packet(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *packet, ENCODE_CALLBACK callback);

#endif //VIDEOBOX_ENCODE_H
