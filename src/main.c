
#include <libswscale/swscale.h>
#include "muxing/demuxing.h"
#include "codec/decode.h"
#include "utils/snapshot.h"
#include "utils/log.h"

#include <stdio.h>
#include <string.h>

FILE *video_file;
FILE *audio_file;
struct SwsContext *sws_ctx = NULL;
int i = 0;

static Media *video_media;
static Media *audio_media;
static AVFrame *video_frame;
static AVFrame *audio_frame;
static Demuxer *demuxer;

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
  snprintf(buf, sizeof(buf), "%s-%d%s", video_file, i, ".ppm");
  save_ppm(data[0], linesize[0], frame->width, frame->height, buf);
  i++;
  av_freep(data);
}

void saveAsYUV(AVFrame *frame) {
  if (!frame)return;
  char buf[256];
  snprintf(buf, sizeof(buf), "%s%03d%s", video_file, i, ".yuv");
  save_yuv(frame->data, frame->linesize, frame->width, frame->height, buf);
  i++;
}

void save_video(AVFrame *frame) {
  for (int j = 0; j < frame->linesize[0]; ++j) {
    fwrite(frame->data[0], frame->height, 1, video_file);
  }
  for (int j = 0; j < frame->linesize[1]; ++j) {
    fwrite(frame->data[1], frame->height / 2, 1, video_file);
  }
  for (int j = 0; j < frame->linesize[2]; ++j) {
    fwrite(frame->data[2], frame->height / 2, 1, video_file);
  }
}

void save_audio(AVFrame *frame) {
  int data_size = av_get_bytes_per_sample(audio_media->codec_ctx->sample_fmt);
  for (int j = 0; j < frame->nb_samples; ++j) {
    for (int ch = 0; ch < frame->channels; ch++)
      fwrite(frame->data[ch] + data_size * i, 1, data_size, video_file);
  }
}

void demux_callback(AVPacket *packet) {
  if (packet && packet->stream_index == video_media->stream_id) {
    LOGD("decoding new packet\n");
    while (decode_packet(video_media->codec_ctx, video_frame, packet, save_video)) {
      LOGD("Decoding video frame: %d\n", video_media->codec_ctx->frame_number);
    }
  }
//  else if (packet && packet->stream_index == audio_media->stream_id) {
//    while (decode_packet(audio_media->codec_ctx, audio_frame, packet, save_audio)) {
//      LOGD("Decoding audio frame: %d\n", audio_media->codec_ctx->frame_number);
//    }
//  }
}

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "demux") == 0) {
      demuxer = get_demuxer(argv[2], NULL, NULL, NULL);
      video_frame = av_frame_alloc();
      audio_frame = av_frame_alloc();
      // find video media
      for (int j = 0; j < demuxer->media_count; ++j) {
        LOGD("stream: %d\n", j);
        if (demuxer->media[j]->media_type == AVMEDIA_TYPE_VIDEO) {
          video_media = demuxer->media[j];
          LOGD("Video Stream at: %d %s\n", j, avcodec_get_name(video_media->codec_ctx->codec_id));
        } else if (demuxer->media[j]->media_type == AVMEDIA_TYPE_AUDIO) {
          audio_media = demuxer->media[j];
          LOGD("Audio Stream at: %d %s\n", j, avcodec_get_name(audio_media->codec_ctx->codec_id));
        }
      }

      if (!video_media) {
        LOGE("Cant find any video stream\n");
      }

      if (!audio_media) {
        LOGE("Cant find any audio stream\n");
      }

      video_file = fopen(argv[3], "wb");
      audio_file = fopen(argv[4], "wb");
      demux(demuxer, demux_callback);

      // flush
      if (video_media)
        decode_packet(video_media->codec_ctx, video_frame, NULL, NULL);
      if (audio_media)
        decode_packet(audio_media->codec_ctx, audio_frame, NULL, NULL);

      fflush(video_file);
      fflush(audio_file);
      fclose(video_file);
      fclose(audio_file);

      free_demuxer(demuxer);
      av_frame_free(&video_frame);
      return 0;
    } else if (strcmp(argv[1], "decode") == 0) {
//      video_file = argv[3];
      decode_h264(argv[2], saveAsPPM);
      if (sws_ctx) sws_freeContext(sws_ctx);
      LOGD("decode done\n");
      return 0;
    }
  }

  fprintf(stderr, "invalid command: ");
  for (int j = 0; j < argc; ++j) {
    printf("%s ", argv[j]);
  }
  printf("\n");
  return -1;
}