//
// Created by Infinity on 2019-04-20.
//

#include "demuxing.h"

#include <stdio.h>

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>


static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static int width, height;
static enum AVPixelFormat pix_fmt;
static AVStream *video_stream = NULL, *audio_stream = NULL;
static const char *src_filename = NULL;
static const char *video_dst_filename = NULL;
static const char *audio_dst_filename = NULL;
static FILE *video_dst_file = NULL;
static FILE *audio_dst_file = NULL;

static uint8_t *video_dst_data[4] = {NULL};
static int video_dst_linesize[4];
static int video_dst_bufsize;

static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket pkt;
static int video_frame_count = 0;
static int audio_frame_count = 0;

static int refcount = 0;

static int media_types[10];

static int decode_packet(int *got_frame, int cached) {
  int ret = 0;
  int decoded = pkt.size;

  *got_frame = 0;

  if (pkt.stream_index == video_stream_idx) {
    ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
    if (ret < 0) {
      fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
      return ret;
    }

    if (*got_frame) {
      if (frame->width != width || frame->height != height || frame->format != pix_fmt) {
        fprintf(stderr, "Error: Width, height and pixel format have to be "
                        "constant in a rawvideo file, but the width, height or "
                        "pixel format of the input video changed:\n"
                        "old: width = %d, height = %d, format = %s\n"
                        "new: width = %d, height = %d, format = %s\n",
                width, height, av_get_pix_fmt_name(pix_fmt),
                frame->width, frame->height,
                av_get_pix_fmt_name(frame->format));
        return -1;
      }

      printf("video_frame%s n:%d coded_n:%d\n",
             cached ? "(cached)" : "",
             video_frame_count++, frame->coded_picture_number);
      av_image_copy(video_dst_data, video_dst_linesize, (const uint8_t **) (frame->data), frame->linesize,
                    pix_fmt, width, height);

      fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
    }
  } else if (pkt.stream_index == audio_stream_idx) {
    ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
    if (ret < 0) {
      fprintf(stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
      return ret;
    }

    decoded = FFMIN(ret, pkt.size);

    if (*got_frame) {
      size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
      const char *channel_name = av_get_channel_name(frame->channel_layout);
      printf("audio_frame%s n:%d nb_samples:%d pts:%s channel: %s\n",
             cached ? "(cached)" : "",
             audio_frame_count++, frame->nb_samples,
             av_ts2timestr(frame->pts, &audio_dec_ctx->time_base),
             channel_name);
      fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
    }
  }

  if (*got_frame && refcount) av_frame_unref(frame);

  return decoded;
}

static int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx,
                              AVFormatContext *av_fmt_ctx, enum AVMediaType type) {
  int ret, stream_index;
  AVStream *st;
  AVCodec *dec = NULL;
  AVDictionary *opts = NULL;

  ret = av_find_best_stream(av_fmt_ctx, type, -1, -1, NULL, 0);
  if (ret < 0) {
    fprintf(stderr, "Could not find %s stream in input file '%s'\n",
            av_get_media_type_string(type), src_filename);
    return ret;
  } else {
    stream_index = ret;
    st = av_fmt_ctx->streams[stream_index];

    /* find decoder for the stream */
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) {
      fprintf(stderr, "Failed to find %s codec\n",
              av_get_media_type_string(type));
      return AVERROR(EINVAL);
    }

    /* Allocate a codec context for the decoder */
    *dec_ctx = avcodec_alloc_context3(dec);
    if (!*dec_ctx) {
      fprintf(stderr, "Failed to allocate the %s codec context\n",
              av_get_media_type_string(type));
      return AVERROR(ENOMEM);
    }

    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
      fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
              av_get_media_type_string(type));
      return ret;
    }

    // if reference counting is set to non-zero, the return value of av_decode_video2() and av_decode_audio4()
    // is always valid before you call av_frame_unref(), otherwise, they are only valid before you call next
    // decode method.
    av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
    if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
      fprintf(stderr, "Failed to open %s codec\n",
              av_get_media_type_string(type));
      return ret;
    }
    *stream_idx = stream_index;
  }
  return 0;
}

static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt) {
  int i;

  struct sample_fmt_entry {
    enum AVSampleFormat sample_fmt;
    const char *fmt_be, *fmt_le;
  } sample_fmt_entries[] = {
          {AV_SAMPLE_FMT_U8,  "u8",    "u8"},
          {AV_SAMPLE_FMT_S16, "s16be", "s16le"},
          {AV_SAMPLE_FMT_S32, "s32be", "s32le"},
          {AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
          {AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
  };

  *fmt = NULL;

  for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
    struct sample_fmt_entry *entry = &sample_fmt_entries[i];
    if (sample_fmt == entry->sample_fmt) {
      *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
      return 0;
    }
  }

  fprintf(stderr, "sample format %s is not supported as output format\n", av_get_sample_fmt_name(sample_fmt));
  return -1;
}

int demux(const char *filename, DEMUX_CALLBACK cb) {

  int ret = 0, got_frame;

  src_filename = filename;

  // open file
  if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
    fprintf(stderr, "Could not open source file %s\n", src_filename);
    exit(1);
  }

  // get video stream info
  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    fprintf(stderr, "Could not find stream information\n");
    exit(1);
  }

  // get decode context
  if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
    video_stream = fmt_ctx->streams[video_stream_idx];

    video_dst_file = fopen(video_dst_filename, "wb");
    if (!video_dst_file) {
      fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
      ret = 1;
      goto end;
    }

    width = video_dec_ctx->width;
    height = video_dec_ctx->height;
    pix_fmt = video_dec_ctx->pix_fmt;
    ret = av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1);
    if (ret < 0) {
      fprintf(stderr, "Could not allocate raw video buffer\n");
      goto end;
    }
    video_dst_bufsize = ret;
  }

  if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
    audio_stream = fmt_ctx->streams[audio_stream_idx];
    audio_dst_file = fopen(audio_dst_filename, "wb");
    if (!audio_dst_file) {
      fprintf(stderr, "Could not open destination file %s\n", audio_dst_filename);
      ret = 1;
      goto end;
    }
  }

  av_dump_format(fmt_ctx, 0, src_filename, 0);

  if (!audio_stream && !video_stream) {
    fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
    ret = 1;
    goto end;
  }

  frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Could not allocate frame\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  av_init_packet(&pkt);
  pkt.data = NULL;
  pkt.size = 0;

  if (video_stream) {
    printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename);
  }
  if (audio_stream) {
    printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename);
  }

  while (av_read_frame(fmt_ctx, &pkt) >= 0) {
    AVPacket orig_pkt = pkt;
    do {
      ret = decode_packet(&got_frame, 0);
      if (ret < 0) {
        break;
      }
      pkt.data += ret;
      pkt.size -= ret;
    } while (pkt.size > 0);
    av_packet_unref(&orig_pkt);
  }

  // flush cache
  pkt.data = NULL;
  pkt.size = 0;
  do {
    decode_packet(&got_frame, 1);
  } while (got_frame);

  printf("Demuxing succeeded.\n");

  if (video_stream) {
    printf("Video stream: format(%d) size(%dx%d)\n", pix_fmt, width, height);
  }

  if (audio_stream) {
    enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
    int n_channels = audio_dec_ctx->channels;
    const char *fmt;

    // if is planar format, channels will be saved in different arrays, not just in AVFrame->data[0]
    // should use linesize to get all channels from AVFrame->data
    if (av_sample_fmt_is_planar(sfmt)) {
      const char *packed = av_get_sample_fmt_name(sfmt);
      printf("warning: planar format(%s) detected, only first channel will be output.\n", packed ? packed : "?");
      sfmt = av_get_packed_sample_fmt(sfmt);
    }

    // detect is format is supported by ffplay
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0) {
      goto end;
    }

    char channel_str[200];

    av_get_channel_layout_string(channel_str, 200, n_channels, audio_dec_ctx->channel_layout);

    printf("Audio stream: format(%s) channel(%d) rate(%d) layout(%s)\n",
           fmt,
           n_channels,
           audio_dec_ctx->sample_rate,
           channel_str);
  }

  end:
  avcodec_free_context(&video_dec_ctx);
  avcodec_free_context(&audio_dec_ctx);
  avformat_close_input(&fmt_ctx);
  if (video_dst_file) fclose(video_dst_file);
  if (audio_dst_file) fclose(audio_dst_file);
  av_frame_free(&frame);
  av_free(video_dst_data[0]);

  return ret < 0;
}
