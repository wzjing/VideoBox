//
// Created by Infinity on 2019-04-20.
//

#ifndef VIDEOBOX_MUXING_H
#define VIDEOBOX_MUXING_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

typedef struct Media {
    uint stream_idx;
    AVMediaType media_type;
    AVCodecContext *codec_ctx;
    AVStream *stream;
    int64_t next_pts;
    AVFrame *frame;
} Media;

typedef struct Muxer {
    const char *output_filename;
    AVFormatContext *fmt_ctx;
    Media **media;
    uint nb_media;

    int *video_ids;
    int *audio_ids;

} Muxer;

Muxer *get_muxer(const char *out_filename);

void add_media(Muxer *muxer, Media *media);

void open_muxer(Muxer *muxer);

int add_packet(Muxer *muxer, AVPacket *packet);

void close_muxer(Muxer *muxer);

#endif //VIDEOBOX_MUXING_H
