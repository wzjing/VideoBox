//
// Created by android1 on 2019/4/25.
//

#ifndef VIDEOBOX_MUX_ENCODE_H
#define VIDEOBOX_MUX_ENCODE_H

//
// Created by android1 on 2019/4/25.
//

#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavutil/mathematics.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "mux_encode.h"
#include "../utils/log.h"
#include "../utils/io.h"
#include "../codec/encode.h"

int mux_encode(const char *filename, const char *video_source, const char *audio_source);

#endif //VIDEOBOX_MUX_ENCODE_H
