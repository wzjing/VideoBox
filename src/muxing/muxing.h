//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_MUXING_H
#define VIDEOBOX_MUXING_H

#include <functional>

#include "mux_comman.h"

//
typedef const std::function<int(AVFrame *frame, int available_media_type)> &MUX_CALLBACK;

Muxer *open_muxer(const char *out_filename);

Media* add_media(Muxer *muxer, MediaConfig *config);

int mux(Muxer *muxer, MUX_CALLBACK callback);

void close_muxer(Muxer *muxer);

#endif //VIDEOBOX_MUXING_H
