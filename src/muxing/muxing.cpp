//
// Created by Infinity on 2019-04-20.
//

#include "muxing.h"
#include "../utils/log.h"

Muxer *open_muxer(const char *out_filename) {
  auto *muxer = (Muxer *) malloc(sizeof(Muxer));
  AVFormatContext *fmt_ctx;
  int ret = 0;

  avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, out_filename);


  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    LOGD("Opening file: %s\n", out_filename);
    ret = avio_open(&fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", out_filename, av_err2str(ret));
      free(muxer);
      return nullptr;
    }
  }

  muxer->output_filename = out_filename;
  muxer->fmt_ctx = fmt_ctx;

  return nullptr;
}

int add_media(Muxer *muxer, MediaConfig *config) {

  if (config == nullptr || muxer == nullptr) {
    LOGE("Error: muxer or media config is nullptr\n");
    return -1;
  }

  int ret;

  auto *media = (Media *) malloc(sizeof(Media));
  AVCodec *codec = avcodec_find_encoder(config->codec_id);
  media->media_type = config->media_type;

  AVStream *stream = avformat_new_stream(muxer->fmt_ctx, nullptr);
  if (!stream) {
    LOGE("could not find create stream\n");
    free(media);
    return -1;
  }
  stream->id = muxer->fmt_ctx->nb_streams - 1;
  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    LOGE("could not alloc encode context\n");
    free(media);
    return -1;
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
        free(media);
        return -1;
      }

      ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
      if (ret < 0) {
        LOGE("unable to copy stream parameter to AVCodecContext: %s\n", av_err2str(ret));
        return -1;
      }

      // alloc frame
      if (!frame) {
        LOGE("unable to alloc frame\n");
        free(media);
        return -1;
      }

      frame->format = config->format;
      frame->width = config->width;
      frame->height = config->height;

      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        LOGE("unable to alloc buffer for frame: %s\n", av_err2str(ret));
        return -1;
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
        free(media);
        return -1;
      }

      ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
      if (ret < 0) {
        LOGE("unable to copy stream parameter to AVCodecContext: %s\n", av_err2str(ret));
        return -1;
      }

      frame->format = config->format;
      frame->channel_layout = config->channel_layout;
      frame->sample_rate = config->sample_rate;
      frame->nb_samples = config->nb_samples;

      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        LOGE("unable to alloc buffer for frame: %s\n", av_err2str(ret));
        return -1;
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
  if (muxer->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }
  media->stream = stream;
  media->codec_ctx = codec_ctx;
  media->media_type = config->media_type;
  media->stream_idx = stream->index;
  media->frame = frame;

  return 1;
}

int mux(Muxer *muxer, MUX_CALLBACK callback) {
  return 0;
}
