#include <cstdio>
#include <cstring>

#include "utils/log.h"
#include "utils/io.h"
#include "demux_decode.h"
#include "mux_encode.h"

int main(int argc, char *argv[]) {

  if (argc > 1) {
    if (strcmp(argv[1], "demux") == 0) {
      demux_decode(argv[2], argv[3], argv[4]);
      return 0;
    } else if (strcmp(argv[1], "mux") == 0) {
      mux_encode(argv[2], argv[3], argv[4]);
      return 0;
    } else if (strcmp(argv[1], "io") == 0) {
      FILE *file = fopen(argv[2], "rb");
      FILE *result = fopen(argv[3], "wb");
      int sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
      int nb_samples = 1024;
      int channels = 2;

      AVFrame *frame = av_frame_alloc();

      frame->format = AV_SAMPLE_FMT_FLTP;
      frame->nb_samples = nb_samples;
      frame->channel_layout = AV_CH_LAYOUT_STEREO;
      frame->sample_rate = 44100;
      av_frame_get_buffer(frame, 0);
      av_frame_make_writable(frame);

      int ret = 0;
      if (!av_frame_is_writable(frame)) {
        LOGE("frame is not writable: %d\n", ret);
        exit(1);
      }
      for (int i = 0; i < 100; ++i) {
        LOGD("frame writable: %s\n", av_frame_is_writable(frame) ? "TRUE" : "FALSE");
        read_pcm(file, frame, nb_samples, channels, i, AV_SAMPLE_FMT_FLTP);
        fwrite(frame->data[0], sample_size, nb_samples, result);
      }

      av_frame_free(&frame);

      fclose(result);
      fclose(file);
      return 0;
    } else if (strcmp(argv[1], "test") == 0) {
      AVFrame *frame = av_frame_alloc();
      frame->format = AV_SAMPLE_FMT_FLTP;
      frame->nb_samples = 1024;
      frame->channel_layout = AV_CH_LAYOUT_STEREO;
      int ret = 0;
      av_frame_get_buffer(frame, 0);
      ret = av_frame_make_writable(frame);
      LOGD("Frame writable: %d %s\n", ret, ret == 0 ? "TRUE" : "FALSE");

      av_frame_unref(frame);
      LOGD("Frame nb_samples: %d\n", frame->nb_samples);
    }
  }

  fprintf(stderr, "invalid command: ");
  for (int j = 0; j < argc; ++j) {
    printf("%s ", argv[j]);
  }
  printf("\n");
  return -1;
}