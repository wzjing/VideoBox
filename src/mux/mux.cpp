//
// Created by Infinity on 2019-04-20.
//

#include "mux.h"
#include "../utils/log.h"
#include "../codec/encode.h"

Muxer *open_muxer(const char *out_filename) {
  auto *muxer = (Muxer *) malloc(sizeof(Muxer));
  AVFormatContext *fmt_ctx;
  AVPacket *packet;

  avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, out_filename);

  packet = av_packet_alloc();

  if (!packet) {
    LOGE("could not alloc AVPacket\n");
    goto error;
  }

  muxer->output_filename = out_filename;
  muxer->fmt_ctx = fmt_ctx;
  muxer->packet = packet;
  return muxer;
  error:
  free(muxer);
  return nullptr;
}

Media *add_media(Muxer *muxer, MediaConfig *config) {

  int ret;
  Media *media = nullptr;
  AVCodec *codec = nullptr;
  AVCodecContext *codec_ctx = nullptr;
  AVStream *stream = nullptr;
  AVFrame *frame = nullptr;

  if (config == nullptr || muxer == nullptr) {
    LOGE("Error: Muxer or MediaConfig is nullptr\n");
    goto error;
  }

  media = (Media *) malloc(sizeof(Media));

  codec = avcodec_find_encoder(config->codec_id);

  stream = avformat_new_stream(muxer->fmt_ctx, nullptr);
  if (!stream) {
    LOGE("could not find create stream\n");
    goto error;
  }

  stream->id = muxer->fmt_ctx->nb_streams - 1;
  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    LOGE("could not alloc encode context\n");
    goto error;
  }

  frame = av_frame_alloc();
  switch (config->media_type) {
    case AVMEDIA_TYPE_VIDEO:
      muxer->video_idx = muxer->nb_media;
      stream->time_base = (AVRational) {1, config->frame_rate};
      codec_ctx->time_base = stream->time_base;
      codec_ctx->pix_fmt = (AVPixelFormat) config->format;
      codec_ctx->codec_id = config->codec_id;
      codec_ctx->bit_rate = config->bit_rate;
      codec_ctx->width = config->width;
      codec_ctx->height = config->height;
      codec_ctx->gop_size = config->gop_size;
      codec_ctx->has_b_frames = 0;

      ret = avcodec_open2(codec_ctx, codec, nullptr);
      if (ret < 0) {
        LOGE("unable to open codec: %s\n", av_err2str(ret));
        free(media);
        goto error;
      }

      ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
      if (ret < 0) {
        LOGE("unable to copy stream parameter to AVCodecContext: %s\n", av_err2str(ret));
        goto error;
      }

      // alloc frame
      if (!frame) {
        LOGE("unable to alloc frame\n");
        free(media);
        goto error;
      }

      frame->format = config->format;
      frame->width = config->width;
      frame->height = config->height;

      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        LOGE("unable to alloc buffer for frame: %s\n", av_err2str(ret));
        goto error;
      }

      break;
    case AVMEDIA_TYPE_AUDIO:
      muxer->audio_idx = muxer->nb_media;
      codec_ctx->sample_fmt = (AVSampleFormat) config->format;
      codec_ctx->codec_id = config->codec_id;
      codec_ctx->bit_rate = config->bit_rate;
      codec_ctx->sample_rate = config->sample_rate;
      codec_ctx->channel_layout = config->channel_layout;
      codec_ctx->channels = av_get_channel_layout_nb_channels(config->channel_layout);
//      codec_ctx->rc_min_rate = config->bit_rate;
//      codec_ctx->rc_max_rate = config->bit_rate;
      stream->time_base = (AVRational) {1, codec_ctx->sample_rate};
      codec_ctx->time_base = stream->time_base;

      ret = avcodec_open2(codec_ctx, codec, nullptr);
      if (ret < 0) {
        LOGE("unable to open codec: %s\n", av_err2str(ret));
        free(media);
        goto error;
      }

      ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
      if (ret < 0) {
        LOGE("unable to copy stream parameter to AVCodecContext: %s\n", av_err2str(ret));
        goto error;
      }

      frame->format = (AVSampleFormat) config->format;
      frame->channel_layout = config->channel_layout;
      frame->channels = av_get_channel_layout_nb_channels(config->channel_layout);
      frame->sample_rate = config->sample_rate;
      frame->nb_samples = config->nb_samples;

      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        LOGE("unable to alloc buffer for frame: %s\n", av_err2str(ret));
        goto error;
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

  media->media_type = config->media_type;
  media->stream = stream;
  media->codec_ctx = codec_ctx;
  media->media_type = config->media_type;
  media->stream_idx = stream->index;
  media->frame = frame;
  muxer->nb_media++;
  muxer->media = (Media **) realloc(muxer->media, muxer->nb_media * sizeof(Media));
  muxer->media[muxer->nb_media - 1] = media;

  return media;

  error:
  if (media) free(media);
  if (codec_ctx) avcodec_free_context(&codec_ctx);
  return nullptr;
}

static void log_packet(AVStream *stream, const AVPacket *pkt) {
  AVRational *time_base = &stream->time_base;
  LOGD("\033[31mstream: #%d -> index:%4ld\tpts:%-8s\tpts_time:%-8s\tdts:%-8s\tdts_time:%-8s\tduration:%-8s\tduration_time:%-8s\033[0m\n",
         stream->index,
         stream->nb_frames,
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base));
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *stream, AVPacket *pkt) {
//  LOGD("\033[32mpacket: pts: %ld dts: %ld timebase: %d/%d stream_timebase: %d/%d\033[0m\n", pkt->pts, pkt->dts, time_base->num, time_base->den, stream->time_base.num, stream->time_base.den);

  log_packet(stream, pkt);
  av_packet_rescale_ts(pkt, *time_base, stream->time_base);
//  if (av_compare_ts(pkt->pts, stream->time_base,
//                    5000, (AVRational) {1, 1}) > 0) {
//    LOGD("Stream #%d finished\n", stream->index);
//    return 1;
//  }
  pkt->stream_index = stream->index;
  log_packet(stream, pkt);
  int ret;
  if ((ret = av_interleaved_write_frame(fmt_ctx, pkt)) != 0) {
    LOGE("write_frame: error write frame: %s\n", av_err2str(ret));
    return -1;
  }

  return 0;
}

int mux(Muxer *muxer, MUX_CALLBACK callback) {
  int ret = 0;

  if (!(muxer->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    LOGD("Opening file: %s\n", muxer->output_filename);
    ret = avio_open(&muxer->fmt_ctx->pb, muxer->output_filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", muxer->output_filename, av_err2str(ret));
      return -1;
    }
  }

  AVDictionary *container_opts = nullptr;
  av_dict_set(&container_opts, "movflags", "rtphint+faststart", 0);


  ret = avformat_write_header(muxer->fmt_ctx, &container_opts);
  if (ret != 0) {
    LOGE("Error while write file header: %s\n", av_err2str(ret));
    return -1;
  }

  Media *video_media = muxer->media[muxer->video_idx];
  Media *audio_media = muxer->media[muxer->audio_idx];

  video_media->input_eof = false;
  video_media->output_eof = false;
  audio_media->input_eof = false;
  audio_media->output_eof = false;

  while (!video_media->output_eof || !audio_media->output_eof) {
    LOGD("Start: Video(%d/%d), Audio:(%d/%d)\n", video_media->input_eof, video_media->output_eof, audio_media->input_eof,
         audio_media->output_eof);
    if (!video_media->output_eof && av_compare_ts(video_media->next_pts,
                                                  video_media->codec_ctx->time_base,
                                                  audio_media->next_pts,
                                                  audio_media->codec_ctx->time_base) <= 0) {
      LOGD("Video PTS: %ld\n", video_media->next_pts);
      if (!video_media->input_eof) {
        video_media->input_eof = callback(video_media->frame, AVMEDIA_TYPE_VIDEO) || audio_media->input_eof;
        video_media->frame->pts = video_media->next_pts;
        video_media->next_pts++;
      } else {
        LOGD("Video flush\n");

      }
      ret = encode_packet(video_media->codec_ctx, video_media->input_eof ? nullptr : video_media->frame, muxer->packet,
                          [&muxer, &video_media](AVPacket *packet) -> int {
                            return write_frame(muxer->fmt_ctx, &video_media->codec_ctx->time_base, video_media->stream,
                                        muxer->packet);
                          });
      LOGD("encode: %d\n", ret);
      if (ret < 0) {
        LOGE("Video encode error\n");
        break;
      }
      video_media->output_eof = ret == 1;
    } else {
      if (!audio_media->input_eof) {
        LOGD("Audio PTS: %ld\n", audio_media->next_pts);
        audio_media->input_eof = callback(audio_media->frame, AVMEDIA_TYPE_AUDIO) || video_media->input_eof;
        audio_media->frame->pts = audio_media->next_pts;
        audio_media->next_pts += audio_media->frame->nb_samples;
      } else {
        LOGD("Audio flush\n");
      }
      ret = encode_packet(audio_media->codec_ctx, audio_media->input_eof ? nullptr : audio_media->frame, muxer->packet,
                          [&muxer, &audio_media](AVPacket *packet) -> int {
                            return write_frame(muxer->fmt_ctx, &audio_media->codec_ctx->time_base, audio_media->stream,
                                        muxer->packet);
                          });
      if (ret < 0) {
        LOGE("Audio encode error\n");
        break;
      }
      audio_media->output_eof = ret == 1;
    }
    LOGD("\n");
  }

  ret = av_write_trailer(muxer->fmt_ctx);
  if (ret != 0) {
    LOGE("Failed to write trailer: %s\n", av_err2str(ret));
    return -1;
  }

  for (int i = 0; i < muxer->nb_media; ++i) {
    Media *media = muxer->media[i];
    avcodec_free_context(&media->codec_ctx);
    av_frame_free(&media->frame);
  }

  if (!(muxer->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    avio_closep(&muxer->fmt_ctx->pb);
  }
  return 0;
}

void close_muxer(Muxer *muxer) {
  if (muxer != nullptr) {
    av_packet_free(&muxer->packet);

    avformat_free_context(muxer->fmt_ctx);
  }
}
