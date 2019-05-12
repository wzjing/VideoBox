//
// Created by android1 on 2019/4/25.
//

#ifndef VIDEOBOX_MUX_ENCODE_HPP
#define VIDEOBOX_MUX_ENCODE_HPP

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

#include "mux_encode.hpp"
#include "utils/log.h"
#include "utils/io.h"
#include "codec/encode.h"

#define STREAM_DURATION 5

typedef struct OutputStream {
  AVStream *stream = nullptr;
  AVCodecContext *codec_ctx = nullptr;

  int64_t next_pts = 0;
  int nb_samples = 0;

  AVFrame *frame = nullptr;
} OutputStream;

static void log_packet(AVStream *stream, const AVPacket *pkt) {
  AVRational *time_base = &stream->time_base;
  printf("stream: #%d -> index:%4ld\tpts:%-8s\tpts_time:%-8s\tdts:%-8s\tdts_time:%-8s\tduration:%-8s\tduration_time:%-8s\n",
         stream->index,
         stream->nb_frames,
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base));
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *stream, AVPacket *pkt) {
  av_packet_rescale_ts(pkt, *time_base, stream->time_base);
  if (av_compare_ts(pkt->pts, stream->time_base,
                    STREAM_DURATION, (AVRational) {1, 1}) > 0) {
    LOGD("Stream #%d finished\n", stream->index);
    return 0;
  }
  pkt->stream_index = stream->index;
  log_packet(stream, pkt);
  int ret;
  if ((ret = av_interleaved_write_frame(fmt_ctx, pkt)) != 0) {
    LOGE("write_frame: error write frame: %s\n", av_err2str(ret));
    exit(1);
  }

  return 1;
}

static void add_stream(struct OutputStream *ost, AVFormatContext *fmt_ctx, AVCodec **codec, enum AVCodecID codec_id) {

  LOGD("Add stream: %s\n", avcodec_get_name(codec_id));

  AVCodecContext *codec_ctx;

  *codec = avcodec_find_encoder(codec_id);

  if (!(*codec)) {
    LOGE("add_stream: could not find encoder for: %s\n", avcodec_get_name(codec_id));
    exit(1);
  }

  ost->stream = avformat_new_stream(fmt_ctx, nullptr);
  if (!ost->stream) {
    LOGE("add_stream: could not create stream\n");
    exit(1);
  }

  ost->stream->id = fmt_ctx->nb_streams - 1;
  codec_ctx = avcodec_alloc_context3(*codec);
  if (!codec_ctx) {
    LOGE("add_stream: could not alloc encode context\n");
    exit(1);
  }

  ost->codec_ctx = codec_ctx;

  switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
      ost->nb_samples = 1024;
      codec_ctx->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
      codec_ctx->bit_rate = 64000;
      codec_ctx->sample_rate = 44100;
      codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
      codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
      ost->stream->time_base = (AVRational) {1, codec_ctx->sample_rate};
      codec_ctx->time_base = ost->stream->time_base;
//      ost->next_pts = ost->nb_samples;
      break;
    case AVMEDIA_TYPE_VIDEO:
      ost->stream->time_base = (AVRational) {1, 30};
//      ost->next_pts = 1;
      codec_ctx->codec_id = codec_id;
      codec_ctx->bit_rate = 4000000;
      codec_ctx->width = 1920;
      codec_ctx->height = 1080;
      codec_ctx->time_base = ost->stream->time_base;
      codec_ctx->gop_size = 12;
      codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
      break;
    case AVMEDIA_TYPE_DATA:
    case AVMEDIA_TYPE_SUBTITLE:
    case AVMEDIA_TYPE_ATTACHMENT:
    case AVMEDIA_TYPE_NB:
    case AVMEDIA_TYPE_UNKNOWN:
      LOGE("add_stream: unknown stream type: %d", (*codec)->type);
      break;
  }

  if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

static void close_stream(OutputStream *ost) {
  avcodec_free_context(&ost->codec_ctx);
  av_frame_free(&ost->frame);
}

static AVFrame *alloc_video_frame(enum AVPixelFormat pix_fmt, int width, int height) {
  AVFrame *frame;
  int ret;

  frame = av_frame_alloc();
  if (!frame)
    return nullptr;

  frame->format = pix_fmt;
  frame->width = width;
  frame->height = height;

  /* allocate the buffers for the frame data */
  ret = av_frame_get_buffer(frame, 0);
  if (ret < 0) {
    fprintf(stderr, "Could not allocate frame data.\n");
    exit(1);
  }

  return frame;
}

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples) {
  AVFrame *frame = av_frame_alloc();
  int ret;

  if (!frame) {
    fprintf(stderr, "Error allocating an audio frame\n");
    exit(1);
  }

  frame->format = sample_fmt;
  frame->channel_layout = channel_layout;
  frame->sample_rate = sample_rate;
  frame->nb_samples = nb_samples;

  if (nb_samples) {
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
      fprintf(stderr, "Error allocating an audio buffer\n");
      exit(1);
    }
  }

  return frame;
}

static void open_video(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
  int ret;
  AVCodecContext *codec_ctx = ost->codec_ctx;
  AVDictionary *opt = nullptr;
  av_dict_copy(&opt, opt_arg, 0);

  ret = avcodec_open2(codec_ctx, codec, &opt);
  av_dict_free(&opt);
  if (ret < 0) {
    LOGE("open_video: could not open video codec: %s\n", av_err2str(ret));
    exit(1);
  }

  ost->frame = alloc_video_frame(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
  if (!ost->frame) {
    LOGE("open_video: unable to allocate video frame\n");
    exit(1);
  }

  ret = avcodec_parameters_from_context(ost->stream->codecpar, codec_ctx);
  if (ret < 0) {
    LOGE("open_video: could not copy the stream parameters\n");
    exit(1);
  }
}

static void open_audio(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
  AVCodecContext *codec_ctx;
  int nb_samples;
  int ret = 0;
  AVDictionary *opt = nullptr;
  codec_ctx = ost->codec_ctx;
  av_dict_copy(&opt, opt_arg, 0);
  ret = avcodec_open2(codec_ctx, codec, &opt);
  av_dict_free(&opt);

  if (ret < 0) {
    LOGE("open_audio: could not open audio codec: %s", av_err2str(ret));
    exit(1);
  }

  if (codec_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
    nb_samples = 1024;
  } else {
    nb_samples = codec_ctx->frame_size;
  }

  ost->frame = alloc_audio_frame(codec_ctx->sample_fmt, codec_ctx->channel_layout,
                                 codec_ctx->sample_rate, nb_samples);

  ret = avcodec_parameters_from_context(ost->stream->codecpar, codec_ctx);
  if (ret < 0) {
    LOGE("open_audio: could not copy the stream parameters\n");
    exit(1);
  }
}

static AVFrame *get_video_frame(OutputStream *ost, FILE *video_file) {
  if (av_compare_ts(ost->next_pts, ost->codec_ctx->time_base,
                    STREAM_DURATION, (AVRational) {1, 1}) >= 0) {
    return nullptr;
  }

  if (av_frame_make_writable(ost->frame) < 0) {
    exit(1);
  }

  // frame data
  read_yuv(video_file, ost->frame, ost->frame->width, ost->frame->height, ost->codec_ctx->frame_number,
           (AVPixelFormat) ost->frame->format);

  ost->frame->pts = ost->next_pts++;
  return ost->frame;
}

static AVFrame *get_audio_frame(OutputStream *ost, FILE *audio_file) {
  if (av_compare_ts(ost->next_pts, ost->codec_ctx->time_base,
                    STREAM_DURATION, (AVRational) {1, 1}) >= 0) {
    return nullptr;
  }

  if (av_frame_make_writable(ost->frame) < 0) {
    exit(1);
  }

  // frame data
  read_pcm(audio_file, ost->frame, ost->frame->nb_samples, ost->frame->channels, ost->codec_ctx->frame_number,
           (AVSampleFormat) ost->frame->format);

  ost->frame->pts = ost->next_pts;
  ost->next_pts += ost->frame->nb_samples;
  return ost->frame;
}

static int encode(AVFormatContext *fmt_ctx, OutputStream *stream, AVFrame *frame) {
  AVPacket packet = {nullptr};
  av_init_packet(&packet);
  int ret = 1;
  auto cb = [&ret, &fmt_ctx, &stream](AVPacket *pkt) -> void {
    if (pkt) {
      ret = write_frame(fmt_ctx, &stream->codec_ctx->time_base, stream->stream,
                        pkt);
    }
  };
  int encode_ret = encode_packet(stream->codec_ctx, frame, &packet, cb);
  return ret && encode_ret != 1;
}

int mux_encode(const char *filename, const char *video_source, const char *audio_source) {
  OutputStream video_stream = {}, audio_stream = {};
//  AVOutputFormat *fmt;
  AVFormatContext *fmt_ctx;
  AVCodec *video_codec, *audio_codec;
  int ret = 0;
  int have_video = 0, have_audio = 0;
  int encode_video = 0, encode_audio = 0;
  AVDictionary *opt = nullptr;

  avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filename);

  if (!fmt_ctx) {
    LOGE("unable to create AVFormatContext\n");
    exit(1);
  }

//  fmt = fmt_ctx->oformat;

  if (fmt_ctx->oformat->video_codec != AV_CODEC_ID_NONE) {
    add_stream(&video_stream, fmt_ctx, &video_codec, fmt_ctx->oformat->video_codec);
    have_video = 1;
    encode_video = 1;
  }

  if (fmt_ctx->oformat->audio_codec != AV_CODEC_ID_NONE) {
    add_stream(&audio_stream, fmt_ctx, &audio_codec, fmt_ctx->oformat->audio_codec);
    have_audio = 1;
    encode_audio = 1;
  }

  if (have_video) {
    open_video(video_codec, &video_stream, opt);
  }

  if (have_audio) {
    open_audio(audio_codec, &audio_stream, opt);
  }

  av_dump_format(fmt_ctx, 0, filename, 1);

  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    LOGD("Opening file: %s\n", filename);
    ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", filename, av_err2str(ret));
      return -1;
    }
  }

  ret = avformat_write_header(fmt_ctx, &opt);
  if (ret < 0) {
    LOGE("error occurred when opening output file %s", av_err2str(ret));
    return -1;
  }

  FILE *video_file = fopen(video_source, "rb");
  FILE *audio_file = fopen(audio_source, "rb");

  while (encode_video || encode_audio) {
    LOGD("---------------------------------start-----------------------------\n");
    LOGD("Video[%d]: %8ld Audio[%d]: %8ld\n", encode_video, video_stream.next_pts,
         encode_audio, audio_stream.next_pts);
    if (encode_video && av_compare_ts(video_stream.next_pts,
                                      video_stream.codec_ctx->time_base,
                                      audio_stream.next_pts,
                                      audio_stream.codec_ctx->time_base) <= 0) {
      AVFrame *frame = get_video_frame(&video_stream, video_file);
      encode_video = encode(fmt_ctx, &video_stream, frame);
    } else if (encode_audio) {
      AVFrame *frame = get_audio_frame(&audio_stream, audio_file);
      encode_audio = encode(fmt_ctx, &audio_stream, frame);
    } else {
      break;
    }
    LOGD("---------------------------------end-----------------------------\n");
  }

  av_write_trailer(fmt_ctx);

  if (have_video) {
    close_stream(&video_stream);
  }
  if (have_audio) {
    close_stream(&audio_stream);
  }

  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    avio_closep(&fmt_ctx->pb);
  }

  avformat_free_context(fmt_ctx);
  return 0;
}

#endif //VIDEOBOX_MUX_ENCODE_HPP
