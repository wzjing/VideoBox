//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_ENCODE_H
#define VIDEOBOX_ENCODE_H

#include <functional>
#include "../utils/log.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/bsf.h>
}

typedef const std::function<int(AVPacket *pkt)> &ENCODE_CALLBACK;

int encode_packet(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *packet, ENCODE_CALLBACK callback);

int encode_packet_bsf(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *packet, AVBSFContext* bsfContext, ENCODE_CALLBACK callback);

#endif //VIDEOBOX_ENCODE_H
