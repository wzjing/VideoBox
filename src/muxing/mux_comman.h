//
// Created by android1 on 2019/5/10.
//

#ifndef VIDEOBOX_MUX_COMMAN_H
#define VIDEOBOX_MUX_COMMAN_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
}

typedef struct MediaConfig {
  AVMediaType media_type;
  AVCodecID codec_id;

  // Pixel format for video, sample format for audio
  int format;

  // bit rate decide the quality, higher is better
  int64_t bit_rate;

  // only for audio
  int sample_rate; // frame rate in byte: 44100, 48000...
  int nb_samples; // samples per frame: 1024 for aac
  int64_t channel_layout;  // channel layout

  // only for video
  int height;
  int width;
  int frame_rate; // frames per second
  int gop_size;
} MediaConfig;

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

  uint video_idx;
  uint audio_idx;

  AVPacket *packet;
} Muxer;

#endif //VIDEOBOX_MUX_COMMAN_H
