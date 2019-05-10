//
// Created by android1 on 2019/5/9.
//

#ifndef VIDEOBOX_MULTI_MUX_HPP
#define VIDEOBOX_MULTI_MUX_HPP

//
// Created by android1 on 2019/5/9.
//

#include "multi-mux.hpp"
#include "utils/log.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}

//typedef struct MediaConfig {
//  AVMediaType media_type;
//  AVCodecID codec_id;
//
//  // Pixel format for video, sample format for audio
//  int format;
//
//  // bit rate decide the quality, higher is better
//  int64_t bit_rate;
//
//  // only for audio
//  int sample_rate; // frame rate in byte: 44100, 48000...
//  int nb_samples; // samples per frame: 1024 for aac
//  int64_t channel_layout;  // channel layout
//
//  // only for video
//  int height;
//  int width;
//  int frame_rate; // frames per second
//  int gop_size;
//} MediaConfig;
//
//typedef struct Media {
//  uint stream_idx;
//  AVMediaType media_type;
//  AVCodecContext *codec_ctx;
//  AVStream *stream;
//  int64_t next_pts;
//  AVFrame *frame;
//} Media;

Media *add_media(AVFormatContext *fmt_ctx, MediaConfig *config) {

  if (config == nullptr) {
    LOGE("Error: muxer or media config is nullptr\n");
    return nullptr;
  }

  int ret;

  auto *media = (Media *) malloc(sizeof(Media));
  AVCodec *codec = avcodec_find_encoder(config->codec_id);
  media->media_type = config->media_type;

  AVStream *stream = avformat_new_stream(fmt_ctx, nullptr);
  if (!stream) {
    LOGE("could not find create stream\n");
    return nullptr;
  }
  stream->id = fmt_ctx->nb_streams - 1;
  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    LOGE("could not alloc encode context\n");
    return nullptr;
  }

  AVFrame *frame = av_frame_alloc();
  switch (config->media_type) {
    case AVMEDIA_TYPE_VIDEO:
      stream->time_base = (AVRational) {1, config->frame_rate};
      codec_ctx->pix_fmt = (AVPixelFormat) config->format;
      codec_ctx->time_base = stream->time_base;
      codec_ctx->codec_id = config->codec_id;
      codec_ctx->bit_rate = config->bit_rate;
      codec_ctx->width = config->width;
      codec_ctx->height = config->height;
      codec_ctx->gop_size = config->gop_size;

      ret = avcodec_open2(codec_ctx, codec, nullptr);
      if (ret < 0) {
        LOGE("unable to open codec: %s\n", av_err2str(ret));
        return nullptr;
      }

      ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
      if (ret < 0) {
        LOGE("unable to copy stream parameter to AVCodecContext: %s\n", av_err2str(ret));
        return nullptr;
      }

      // alloc frame
      if (!frame) {
        LOGE("unable to alloc frame\n");
        return nullptr;
      }

      frame->format = config->format;
      frame->width = config->width;
      frame->height = config->height;

      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        LOGE("unable to alloc buffer for frame: %s\n", av_err2str(ret));
        return nullptr;
      }

      break;
    case AVMEDIA_TYPE_AUDIO:
      codec_ctx->sample_fmt = (AVSampleFormat) config->format;
      codec_ctx->codec_id = config->codec_id;
      codec_ctx->bit_rate = config->bit_rate;
      codec_ctx->sample_rate = config->sample_rate;
      codec_ctx->channel_layout = config->channel_layout;
      codec_ctx->channels = av_get_channel_layout_nb_channels(config->channel_layout);
      stream->time_base = (AVRational) {1, codec_ctx->sample_rate};
      codec_ctx->time_base = stream->time_base;

      ret = avcodec_open2(codec_ctx, codec, nullptr);
      if (ret < 0) {
        LOGE("unable to open codec: %s\n", av_err2str(ret));
        return nullptr;
      }

      ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
      if (ret < 0) {
        LOGE("unable to copy stream parameter to AVCodecContext: %s\n", av_err2str(ret));
        return nullptr;
      }

      frame->format = config->format;
      frame->channel_layout = config->channel_layout;
      frame->sample_rate = config->sample_rate;
      frame->nb_samples = config->nb_samples;

      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        LOGE("unable to alloc buffer for frame: %s\n", av_err2str(ret));
        return nullptr;
      }
      break;
    case AVMEDIA_TYPE_DATA:
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      break;
    case AVMEDIA_TYPE_ATTACHMENT:
      break;
    case AVMEDIA_TYPE_NB:
      break;
    case AVMEDIA_TYPE_UNKNOWN:
      break;
  }
  if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  media->stream = stream;
  media->codec_ctx = codec_ctx;
  media->media_type = config->media_type;
  media->stream_idx = stream->index;
  media->frame = frame;

  return media;
}

int open_h264(const char *filename, AVFormatContext *fmt_ctx) {
  int ret = 0;
  ret = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr);
  if (ret < 0) {
    LOGE("get_demuxer: Could not open source file %s %s\n", filename, av_err2str(ret));
    return -1;
  }

  if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
    LOGE("get_demuxer: Could not find stream information\n");
    return -1;
  }

  int have_video = 0;
  for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
    if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      have_video = 1;
      break;
    }
  }

  if (!have_video) {
    LOGE("unable to read video from h264 file: %s\n", filename);
    return -1;
  }
  return 0;
}

static void log_packet(AVStream *stream, const AVPacket *pkt) {
  AVRational *time_base = &stream->time_base;
  printf("stream: #%d -> index:%4ld\tpts:%-8s\tpts_time:%-8s\tdts:%-8s\tdts_time:%-8s\tduration:%-8s\tduration_time:%-8s\n",
         stream->index,
         stream->nb_frames,
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base));
}

static int write_packet(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *stream, AVPacket *pkt) {
  av_packet_rescale_ts(pkt, *time_base, stream->time_base);
  pkt->stream_index = stream->index;
  // TODO: reset pts?
  log_packet(stream, pkt);
  int ret;
  if ((ret = av_interleaved_write_frame(fmt_ctx, pkt)) != 0) {
    LOGE("write_frame: error write frame: %s\n", av_err2str(ret));
    exit(1);
  }

  return 0;
}

int mux_multi(char **video_source, char **audio_source,
              char *result) {
  AVFormatContext *fmt_ctx = nullptr;
  int ret = 0;

  avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, result);


  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    LOGD("Opening file: %s\n", result);
    ret = avio_open(&fmt_ctx->pb, result, AVIO_FLAG_WRITE);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", result, av_err2str(ret));
      return -1;
    }
  }
  MediaConfig video_config;
//  MediaConfig audio_config;
  video_config.media_type = AVMEDIA_TYPE_VIDEO;
  video_config.codec_id = AV_CODEC_ID_H264;
  video_config.width = 720;
  video_config.height = 1280;
  video_config.bit_rate = 72200000;
  video_config.format = AV_PIX_FMT_YUV420P;
  video_config.frame_rate = 30;
  video_config.gop_size = 12;
  Media *video_media = add_media(fmt_ctx, &video_config);
  audio_config.media_type = AVMEDIA_TYPE_AUDIO;
  audio_config.codec_id = AV_CODEC_ID_AAC;
  audio_config.format = AV_SAMPLE_FMT_S16P;
  audio_config.bit_rate = 320;
  audio_config.sample_rate = 44100;
  audio_config.nb_samples = 1024;
  audio_config.channel_layout = AV_CH_LAYOUT_STEREO;
  Media *audio_media = add_media(fmt_ctx, &audio_config);

  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    LOGD("Opening file: %s\n", result);
    ret = avio_open(&fmt_ctx->pb, result, AVIO_FLAG_WRITE);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", result, av_err2str(ret));
      return -1;
    }
  }

  ret = avformat_write_header(fmt_ctx, nullptr);
  if (ret < 0) {
    LOGE("error occurred when opening output file %s", av_err2str(ret));
    return -1;
  }

//  FILE *video_file = fopen(video_source[0], "rb");
//  FILE *audio_file = fopen(audio_source[0], "rb");

  AVFormatContext *v_fmt_ctx = nullptr;
  ret = avformat_open_input(&v_fmt_ctx, video_source[0], nullptr, nullptr);
  if (ret < 0) {
    LOGE("get_demuxer: Could not open source file %s %s\n", video_source[0], av_err2str(ret));
    return -1;
  }

  if (avformat_find_stream_info(v_fmt_ctx, nullptr) < 0) {
    LOGE("get_demuxer: Could not find stream information\n");
    return -1;
  }

  int have_video = 0;
  for (int i = 0; i < v_fmt_ctx->nb_streams; ++i) {
    if (v_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      have_video = 1;
      break;
    }
  }

  if (!have_video) {
    LOGE("unable to read video from h264 file: %s\n", video_source[0]);
    return -1;
  }

  LOGD("streams: %d\n", v_fmt_ctx->nb_streams);

  int encode_video = 1;
  int encode_audio = 1;
  AVPacket v_pkt;
  int64_t video_pts = 0;
  while (true) {
//    if (encode_video && av_compare_ts(video_media->next_pts,
//                                      video_media->codec_ctx->time_base,
//                                      audio_media->next_pts,
//                                      audio_media->codec_ctx->time_base) <= 0) {
    ret = av_read_frame(v_fmt_ctx, &v_pkt);
    if (ret != 0) {
      LOGD("Video EOF");
      break;
    }
    v_pkt.pts = video_pts;
    v_pkt.dts = video_pts;
    video_pts++;
    AVPacket packet = v_pkt;

    ret = write_packet(fmt_ctx, &video_media->codec_ctx->time_base, video_media->stream, &packet);
    if (ret != 0) {
      LOGE("error write video packet");
      break;
    }
    video_pts++;
    av_packet_unref(&packet);
//    } else {
//      // TODO: write audio
//    }
  }

  av_write_trailer(fmt_ctx);

  // free AVCodecContext and AVFrame

  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    avio_closep(&fmt_ctx->pb);
  }

  avformat_free_context(fmt_ctx);

  return 0;
}


#endif //VIDEOBOX_MULTI_MUX_HPP
