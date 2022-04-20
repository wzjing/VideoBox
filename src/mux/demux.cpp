//
// Created by Infinity on 2019-04-20.
//

#include "demux.h"

#include <cstdio>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
}

void free_demuxer(Muxer *demuxer) {
  for (int i = 0; i < demuxer->fmt_ctx->nb_streams; ++i) {
    if (demuxer->media[i]) {
      avcodec_free_context(&demuxer->media[i]->codec_ctx);
    }
  }
  avformat_close_input(&demuxer->fmt_ctx);
}

Media *get_media(AVFormatContext *fmt_ctx, enum AVMediaType media_type, AVDictionary *opt) {
  auto *media = (Media *) malloc(sizeof(Media));
  media->media_type = media_type;

  media->stream_idx = av_find_best_stream(fmt_ctx, media_type, -1, -1, nullptr, 0);

  if (media->stream_idx >= 0) {
    media->stream = fmt_ctx->streams[media->stream_idx];
    const AVCodec *codec = avcodec_find_decoder(media->stream->codecpar->codec_id);

    if (!codec) {
      LOGE("get_media: failed to find codec of stream %d %s",
           media->stream_idx, av_get_media_type_string(media_type));
      return nullptr;
    }

    media->codec_ctx = avcodec_alloc_context3(codec);

    if (!media->codec_ctx) {
      LOGE("get_media: failed to allocate memory for AVCodecContext of %d %s",
           media->stream_idx, av_get_media_type_string(media_type));
      return nullptr;
    }

    if (avcodec_parameters_to_context(media->codec_ctx, media->stream->codecpar)) {
      LOGE("get_media: failed to copy parameters to AVCodecContext of %d %s",
           media->stream_idx, av_get_media_type_string(media_type));
      return nullptr;
    }

    if (avcodec_open2(media->codec_ctx, codec, &opt) < 0) {
      LOGE("get_media: failed to open AVCodecContext of %d %s",
           media->stream_idx, av_get_media_type_string(media_type));
      return nullptr;
    }

    return media;
  } else {
    LOGE("get_media: no stream found for media type %s", av_get_media_type_string(media_type));
    free(media);
    return nullptr;
  }
}

Muxer *get_demuxer(const char *filename, AVDictionary *fmt_open_opt, AVDictionary *fmt_stream_opt,
                   AVDictionary *codec_opt) {
  auto *demuxer = (Muxer *) malloc(sizeof(Muxer));
  demuxer->fmt_ctx = nullptr;
  demuxer->nb_media = 0;

  int ret = 0;
  ret = avformat_open_input(&(demuxer->fmt_ctx), filename, nullptr, &fmt_open_opt);
  if (ret < 0) {
    LOGE("get_demuxer: Could not open source file %s %s\n", filename, av_err2str(ret));
    return nullptr;
  }

  if (avformat_find_stream_info(demuxer->fmt_ctx, &fmt_stream_opt) < 0) {
    LOGE("get_demuxer: Could not find stream information\n");
    return nullptr;
  }

  if (demuxer->fmt_ctx->nb_streams == 0) {
    LOGE("get_demuxer: no stream found\n");
    avformat_free_context(demuxer->fmt_ctx);
    return nullptr;
  }

  demuxer->media = (Media **) malloc(sizeof(Media) * demuxer->fmt_ctx->nb_streams);

  // get all AVCodecContext
  for (int i = 0; i < demuxer->fmt_ctx->nb_streams; ++i) {
    auto *media = (Media *) malloc(sizeof(Media));
    media->stream_idx = i;
    media->stream = demuxer->fmt_ctx->streams[i];
    media->media_type = media->stream->codecpar->codec_type;

    const AVCodec *codec = avcodec_find_decoder(media->stream->codecpar->codec_id);

    if (!codec) {
      LOGE("get_media: failed to find codec of stream %d %s\n",
           media->stream_idx, av_get_media_type_string(media->media_type));
      free(media);
      continue;
    }

    media->codec_ctx = avcodec_alloc_context3(codec);

    if (!media->codec_ctx) {
      LOGE("get_media: failed to allocate memory for AVCodecContext of %d %s\n",
           media->stream_idx, av_get_media_type_string(media->media_type));
      free(media);
      continue;
    }

    if (avcodec_parameters_to_context(media->codec_ctx, media->stream->codecpar)) {
      LOGE("get_media: failed to copy parameters to AVCodecContext of %d %s\n",
           media->stream_idx, av_get_media_type_string(media->media_type));
      free(media);
      continue;
    }

    if (avcodec_open2(media->codec_ctx, codec, &codec_opt) < 0) {
      LOGE("get_media: failed to open AVCodecContext of %d %s\n",
           media->stream_idx, av_get_media_type_string(media->media_type));
      free(media);
      continue;
    }
    demuxer->media[i] = media;
    demuxer->nb_media++;
  }

  return demuxer;
}

void demux(Muxer *demuxer, DEMUX_CALLBACK callback) {
  AVPacket *pkt = av_packet_alloc();
  pkt->size = 0;
  pkt->data = nullptr;

  while (av_read_frame(demuxer->fmt_ctx, pkt) == 0) {
    AVPacket *packet = pkt;
// Do something like:
//    if (pkt.size) {
//      decode_packet();
//      ...
//    }
    callback(packet);
    av_packet_unref(packet);
  }
}
