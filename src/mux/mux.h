//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_MUX_H
#define VIDEOBOX_MUX_H

#include <functional>

#include "mux_comman.h"

//
typedef const std::function<int(AVFrame *frame, int available_media_type)> &MUX_CALLBACK;

Muxer *create_muxer(const char *out_filename);

Media* add_media(Muxer *muxer, MediaConfig *config, AVDictionary *codec_opt);

int mux(Muxer *muxer, MUX_CALLBACK callback, AVDictionary * opt);

void close_muxer(Muxer *muxer);

#endif //VIDEOBOX_MUX_H
