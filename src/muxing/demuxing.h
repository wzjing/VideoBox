//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_DEMUXING_H
#define VIDEOBOX_DEMUXING_H

#include <libavformat/avformat.h>

typedef void (*DEMUX_CALLBACK)(AVPacket * packet, enum AVMediaType type);

int demux(const char *filename, DEMUX_CALLBACK cb);

#endif //VIDEOBOX_DEMUXING_H
