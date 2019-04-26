//
// Created by android1 on 2019/4/25.
//

#include "mux_encode.h"
#include "utils/log.h"
#include "codec/encode.h"

#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavutil/mathematics.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define STREAM_DURATION 10
#define SCALE_FLAGS SWS_BICUBIC

typedef struct OutputStream {
  AVStream *stream;
  AVCodecContext *codec_ctx;

  int64_t next_pts;
  int samples_count;

  AVFrame *frame;
  AVFrame *temp_frame;

  float t, tincr, tincr2;

  struct SwsContext *sws_ctx;
  struct SwrContext *swr_ctx;
} OutputStream;

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
  /* rescale output packet timestamp values from codec to stream timebase */
  av_packet_rescale_ts(pkt, *time_base, st->time_base);
  pkt->stream_index = st->index;

  /* Write the compressed frame to the media file. */
//  log_packet(fmt_ctx, pkt);
  return av_interleaved_write_frame(fmt_ctx, pkt);
}

static void add_stream(struct OutputStream *ost, AVFormatContext *fmt_ctx, AVCodec **codec, enum AVCodecID codec_id) {
  AVCodecContext *codec_ctx;

  *codec = avcodec_find_encoder(codec_id);

  if (!(*codec)) {
    LOGE("add_stream: could not find encoder for: %s\n", avcodec_get_name(codec_id));
    exit(1);
  }

  ost->stream = avformat_new_stream(fmt_ctx, NULL);
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
      codec_ctx->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
      codec_ctx->bit_rate = 64000;
      codec_ctx->sample_rate = 44100;
      // check if sample rate is supported
      if ((*codec)->supported_samplerates) {
        codec_ctx->sample_rate = (*codec)->supported_samplerates[0];
        for (int i = 0; (*codec)->supported_samplerates[i]; ++i) {
          if ((*codec)->supported_samplerates[i] == 44100) {
            codec_ctx->sample_rate = 44100;
          }
        }
      }
      codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
      codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
      if ((*codec)->channel_layouts) {
        codec_ctx->channel_layout = (*codec)->channel_layouts[0];
        for (int i = 0; (*codec)->channel_layouts[i]; ++i) {
          if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO) {
            codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
          }
        }
      }
      codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
      ost->stream->time_base = (AVRational) {1, codec_ctx->sample_rate};
      break;
    case AVMEDIA_TYPE_VIDEO:codec_ctx->codec_id = codec_id;
      codec_ctx->bit_rate = 400000;
      codec_ctx->width = 1920;
      codec_ctx->height = 1080;
      ost->stream->time_base = (AVRational) {1, 30};
      codec_ctx->gop_size = 12;
      codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
      break;
    case AVMEDIA_TYPE_DATA:
    case AVMEDIA_TYPE_SUBTITLE:
    case AVMEDIA_TYPE_ATTACHMENT:
    case AVMEDIA_TYPE_NB:
    case AVMEDIA_TYPE_UNKNOWN:LOGE("add_stream: unknown stream type: %d", (*codec)->type);
      break;
  }
}

static void close_stream(AVFormatContext *oc, OutputStream *ost) {
  avcodec_free_context(&ost->codec_ctx);
  av_frame_free(&ost->frame);
  av_frame_free(&ost->temp_frame);
  sws_freeContext(ost->sws_ctx);
  swr_free(&ost->swr_ctx);
}

static AVFrame *alloc_video_frame(enum AVPixelFormat pix_fmt, int width, int height) {
  AVFrame *frame;
  int ret;

  frame = av_frame_alloc();
  if (!frame)
    return NULL;

  frame->format = pix_fmt;
  frame->width = width;
  frame->height = height;

  /* allocate the buffers for the frame data */
  ret = av_frame_get_buffer(frame, 32);
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

static void open_video(AVFormatContext *fmt_ctx, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
  int ret;
  AVCodecContext *codec_ctx = ost->codec_ctx;
  AVDictionary *opt = NULL;
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

  ost->temp_frame = NULL;
  if (codec_ctx->pix_fmt != AV_PIX_FMT_YUV420P) {
    ost->temp_frame = alloc_video_frame(AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
    if (!ost->temp_frame) {
      LOGE("open_video: unable to allocate temp video frame\n");
      exit(1);
    }
  }

  ret = avcodec_parameters_from_context(ost->stream->codecpar, codec_ctx);
  if (ret < 0) {
    LOGE("open_video: could not copy the stream parameters\n");
    exit(1);
  }
}

static void open_audio(AVFormatContext *fmt_ctx, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
  AVCodecContext *codec_ctx;
  int nb_samples;
  int ret = 0;
  AVDictionary *opt = NULL;
  codec_ctx = ost->codec_ctx;
  av_dict_copy(&opt, opt_arg, 0);
  ret = avcodec_open2(codec_ctx, codec, &opt);
  av_dict_free(&opt);

  if (ret < 0) {
    LOGE("open_audio: could not open audio codec: %s", av_err2str(ret));
    exit(1);
  }

  ost->t = 0;
  ost->tincr = 2 * M_PI * 110.0 / codec_ctx->sample_rate;
  ost->tincr2 = 2 * M_PI * 110.0 / codec_ctx->sample_rate / codec_ctx->sample_rate;

  if (codec_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
    nb_samples = 100000;
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

  ost->swr_ctx = swr_alloc();
  if (!ost->swr_ctx) {
    LOGE("open_audio: could not allocate resampler context\n");
    exit(1);
  }

  av_opt_set_int(ost->swr_ctx, "in_channel_count", codec_ctx->channels, 0);
  av_opt_set_int(ost->swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
  av_opt_set_int(ost->swr_ctx, "out_channel_count", codec_ctx->channels, 0);
  av_opt_set_int(ost->swr_ctx, "out_sample_rate", codec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", codec_ctx->sample_fmt, 0);

  if (swr_init(ost->swr_ctx) < 0) {
    fprintf(stderr, "open_audio: Failed to initialize the resampling context\n");
    exit(1);
  }
}

static AVFrame *get_video_frame(OutputStream *ost) {
  AVCodecContext *c = ost->codec_ctx;

  /* check if we want to generate more frames */
  if (av_compare_ts(ost->next_pts, c->time_base,
                    STREAM_DURATION, (AVRational) {1, 1}) >= 0)
    return NULL;

  /* when we pass a frame to the encoder, it may keep a reference to it
   * internally; make sure we do not overwrite it here */
  if (av_frame_make_writable(ost->frame) < 0)
    exit(1);

  // frame data

  ost->frame->pts = ost->next_pts++;

  return ost->frame;
}

void mux_encode(const char *filename, const char *video_source, const char *audio_source) {
  OutputStream video_stream = {0}, audio_stream = {};
  AVOutputFormat *fmt;
  AVFormatContext *fmt_ctx;
  AVCodec *video_codec, *audio_codec;
  int ret = 0;
  int have_video = 0, have_audio = 0;
  int encode_video = 0, encode_audio = 0;
  AVDictionary *opt = NULL;
  int i;

  avformat_alloc_output_context2(&fmt_ctx, NULL, "MPEG-4", filename);

  if (!fmt_ctx) {
    LOGE("unable to create AVFormatContext\n");
    exit(1);
  }

  fmt = fmt_ctx->oformat;

  if (fmt->video_codec != AV_CODEC_ID_NONE) {
    add_stream(&video_stream, fmt_ctx, &video_codec, fmt->video_codec);
    have_video = 1;
    encode_video = 1;
  }

  if (fmt->audio_codec != AV_CODEC_ID_NONE) {
    add_stream(&audio_stream, fmt_ctx, &audio_codec, fmt->video_codec);
    have_audio = 1;
    encode_audio = 1;
  }

  if (have_video) {
    open_video(fmt_ctx, video_codec, &video_stream, opt);
  }

  if (have_audio) {
    open_audio(fmt_ctx, audio_codec, &audio_stream, opt);
  }

  av_dump_format(fmt_ctx, 0, filename, 1);

  if (!(fmt->flags & AVFMT_NOFILE)) {
    ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_DIRECT);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", filename, av_err2str(ret));
      return;
    }
  }

  ret = avformat_write_header(fmt_ctx, &opt);
  if (ret < 0) {
    LOGE("error occurred when opening output file %s", av_err2str(ret));
    return;
  }

  while (encode_video || encode_audio) {
    if (encode_video &&
        (!encode_audio ||
            av_compare_ts(video_stream.next_pts,
                          video_stream.codec_ctx->time_base,
                          audio_stream.next_pts,
                          audio_stream.codec_ctx->time_base) <= 0)) {
      AVPacket packet = {nullptr};
      av_init_packet(&packet);
      // TODO: fill the AVFrame
      encode_packet(video_stream.codec_ctx, video_stream.frame, &packet, [](AVPacket *pkt) -> void {

      });

    } else {
      // TODO: encode audio and set encode_audio
    }
  }

  av_write_trailer(fmt_ctx);

  if (have_video) {
    close_stream(fmt_ctx, &video_stream);
  }
  if (have_audio) {
    close_stream(fmt_ctx, &audio_stream);
  }

  if (!(fmt->flags & AVFMT_NOFILE)) {
    avio_closep(&fmt_ctx->pb);
  }

  avformat_free_context(fmt_ctx);
  LOGD("Muxing done!\n");
}

