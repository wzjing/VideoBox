
#include <libswscale/swscale.h>
#include "muxing/demuxing.h"
#include "codec/decode.h"
#include "utils/snapshot.h"
#include "utils/log.h"

#include <stdio.h>
#include <string.h>

const char *filename;
struct SwsContext *sws_ctx = NULL;
int i = 0;

static Media *video;
static AVFrame *av_frame;
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
  snprintf(buf, sizeof(buf), "%s-%d%s", filename, i, ".ppm");
  save_ppm(data[0], linesize[0], frame->width, frame->height, buf);
  i++;
  av_freep(data);
}

void saveAsYUV(AVFrame * frame) {
  if (!frame)return;
  char buf[256];
  snprintf(buf, sizeof(buf), "%s-%d%s", filename, i, ".yuv");
  save_yuv(frame->data, frame->linesize, frame->width, frame->height, buf);
  i++;
}

void demux_callback(AVPacket *packet) {
  if (packet && packet->stream_index == video->stream_id) {
    LOGD("Decoding Frame...\n");
    while (packet->size)
      decode_packet(video->codec_ctx, av_frame, packet, saveAsYUV);
    LOGD("Decoding Finished\n");
  }
}

int main(int argc, char *argv[]) {

  if (argc > 1) {

    if (strcmp(argv[1], "demux") == 0) {
      filename = argv[3];
      demuxer = get_demuxer(argv[2], NULL, NULL, NULL);
      av_frame = av_frame_alloc();
      // find video media
      for (int j = 0; j < demuxer->media_count; ++j) {
        if (demuxer->media[j]->media_type == AVMEDIA_TYPE_VIDEO) {
          LOGD("Video Stream at: %d\n", j);
          video = demuxer->media[j];
        } else {
          LOGD("Other stream: %d\n", j);
        }
      }

      if (!video) {
        LOGE("Cant find any video stream\n");
        return -1;
      }

      demux(demuxer, demux_callback);
      decode_packet(video->codec_ctx, av_frame, NULL, NULL);
      free_demuxer(demuxer);
      av_frame_free(&av_frame);
      return 0;
    } else if (strcmp(argv[1], "decode") == 0) {
      filename = argv[3];
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