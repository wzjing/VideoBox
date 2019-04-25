//
// Created by android1 on 2019/4/25.
//

#include "demux_decode.h"
#include "muxing/demuxing.h"
#include "codec/decode.h"
#include "utils/snapshot.h"
#include "utils/log.h"

#include <stdio.h>
#include <libswscale/swscale.h>

static const char *frame_name;
static FILE *video_file;
static FILE *audio_file;

static Media *video_media;
static Media *audio_media;
static AVFrame *video_frame;
static AVFrame *audio_frame;
static Demuxer *demuxer;

// Additional image format convert
static struct SwsContext *sws_ctx = NULL;

void saveAsPPM(AVFrame *frame) {
  if (!frame) return;
  uint8_t *data[4];
  int linesize[4];

  if (!sws_ctx) {
    sws_ctx = sws_getContext(frame->width, frame->height, frame->format,
                             frame->width, frame->height, AV_PIX_FMT_RGB24,
                             SWS_BILINEAR, NULL, NULL, NULL);
  }

  if (av_image_alloc(data, linesize, frame->width, frame->height, AV_PIX_FMT_RGB24, 1) < 0) {
    LOGE("scale: Unable to allocate an temp data\n");
    exit(1);
  }

  sws_scale(sws_ctx, (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height, data,
            linesize);

  char buf[256];
  snprintf(buf, sizeof(buf), "%s-%d%s", frame_name, video_media->codec_ctx->frame_number, ".ppm");
  save_ppm(data[0], linesize[0], frame->width, frame->height, buf);
  av_freep(data);
}

void saveAsYUV(AVFrame *frame) {
  if (!frame)return;
  LOGD("Saving frame: %d\n", video_media->codec_ctx->frame_number);
  char buf[256];
  snprintf(buf, sizeof(buf), "%s%03d%s", frame_name, video_media->codec_ctx->frame_number, ".yuv");
  save_yuv(frame->data, frame->linesize, frame->width, frame->height, buf);
  LOGD("Frame saved\n");
}

void save_video(AVFrame *frame) {
  if (!frame) return;
  LOGD("Saving frame: %02d", video_media->codec_ctx->frame_number);
  long start = ftell(video_file);
  for (int i = 0; i < frame->height; i++) {
    fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->linesize[0], video_file);
  }
  LOGD("\t0: %ld\t1: %ld", start, ftell(video_file) - start);
  start = ftell(video_file);
  for (int i = 0; i < frame->height / 2; i++) {
    fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->linesize[1], video_file);
  }
  LOGD("\t2: %ld", ftell(video_file) - start);
  start = ftell(video_file);
  for (int i = 0; i < frame->height / 2; i++) {
    fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->linesize[2], video_file);
  }
  LOGD("\t3: %ld\n", ftell(video_file) - start);
}

void save_audio(AVFrame *frame) {
  LOGD("Track: %d Channel: %d\n", frame->nb_samples, frame->channels);
  int data_size = av_get_bytes_per_sample(audio_media->codec_ctx->sample_fmt);
  for (int i = 0; i < frame->nb_samples; ++i) {
    for (int ch = 0; ch < frame->channels; ch++)
      fwrite(frame->data[ch] + data_size * i, 1, data_size, audio_file);
  }
}

void demux_callback(AVPacket *packet) {
  if (packet && packet->stream_index == video_media->stream_id) {
    int ret = 0;
    do {
      ret = decode_packet(video_media->codec_ctx, video_frame, packet, save_video);
    } while (ret);
  } else if (packet && packet->stream_index == audio_media->stream_id) {
    int ret = 0;
    do {
      ret = decode_packet(audio_media->codec_ctx, audio_frame, packet, save_audio);
      LOGD("Decoding audio frame: %d\n", audio_media->codec_ctx->frame_number);
    } while (ret);
  }
}

void demux_decode(const char *input_filename, const char *output_video_name, const char *output_audio_name) {
  demuxer = get_demuxer(input_filename, NULL, NULL, NULL);
  video_frame = av_frame_alloc();
  audio_frame = av_frame_alloc();
  // find media
  for (int i = 0; i < demuxer->media_count; ++i) {
    LOGD("stream: %d\n", i);
    if (demuxer->media[i]->media_type == AVMEDIA_TYPE_VIDEO) {
      video_media = demuxer->media[i];
      LOGD("Video Stream at: %d %s\n", i, avcodec_get_name(video_media->codec_ctx->codec_id));
    } else if (demuxer->media[i]->media_type == AVMEDIA_TYPE_AUDIO) {
      audio_media = demuxer->media[i];
      LOGD("Audio Stream at: %d %s\n", i, avcodec_get_name(audio_media->codec_ctx->codec_id));
    }
  }

  if (!video_media) {
    LOGE("Cant find any video stream\n");
  }

  if (!audio_media) {
    LOGE("Cant find any audio stream\n");
  }

  video_file = fopen(output_video_name, "wb");
  audio_file = fopen(output_audio_name, "wb");
  demux(demuxer, demux_callback);

  // flush
  if (video_media)
    decode_packet(video_media->codec_ctx, video_frame, NULL, NULL);
  if (audio_media)
    decode_packet(audio_media->codec_ctx, audio_frame, NULL, NULL);

  fflush(stdout);
  fflush(audio_file);
  fclose(video_file);
  fclose(audio_file);

  free_demuxer(demuxer);
  av_frame_free(&video_frame);
  av_frame_free(&audio_frame);
}
