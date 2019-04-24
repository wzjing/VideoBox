//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_DEMUXING_H
#define VIDEOBOX_DEMUXING_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "../utils/log.h"

typedef struct Media {
  unsigned int stream_id;
  enum AVMediaType media_type;
  AVCodecContext* codec_ctx;
  AVStream * stream;
} Media;

typedef struct Demuxer {
  AVFormatContext* fmt_ctx;
  Media **media;
  unsigned int media_count;
} Demuxer;


typedef void (*DEMUX_CALLBACK)(AVPacket *packet);

Demuxer *get_demuxer(const char *filename, AVDictionary *fmt_open_opt, AVDictionary *fmt_stream_opt,
                     AVDictionary *codec_opt);

void demux(Demuxer *demuxer, DEMUX_CALLBACK callback);

void free_demuxer(Demuxer *demuxer);

#endif //VIDEOBOX_DEMUXING_H
