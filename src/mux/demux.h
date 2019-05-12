//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_DEMUX_H
#define VIDEOBOX_DEMUX_H

#include "../utils/log.h"
#include <functional>
#include "mux_comman.h"


typedef const std::function<void(AVPacket *packet)> &DEMUX_CALLBACK;

Muxer *get_demuxer(const char *filename, AVDictionary *fmt_open_opt, AVDictionary *fmt_stream_opt,
                     AVDictionary *codec_opt);

void demux(Muxer *demuxer, DEMUX_CALLBACK callback);

void free_demuxer(Muxer *demuxer);

#endif //VIDEOBOX_DEMUX_H
