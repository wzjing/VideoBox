//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_ENCODE_H
#define VIDEOBOX_ENCODE_H

#include <libavcodec/avcodec.h>
#include "../utils/log.h"

typedef void(*ENCODE_CALLBACK)(AVPacket * packet);

void encode_packet(AVCodecContext * enc_ctx, AVFrame * frame, AVPacket * packet, ENCODE_CALLBACK callback);

#endif //VIDEOBOX_ENCODE_H
