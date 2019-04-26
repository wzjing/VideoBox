#include "utils/log.h"
#include "demux_decode.h"
#include "utils/io.h"

#include <cstdio>
#include <cstring>

int main(int argc, char *argv[]) {

  if (argc > 1) {
    if (strcmp(argv[1], "demux") == 0) {
      demux_decode(argv[2], argv[3], argv[4]);
      return 0;
    } else if (strcmp(argv[1], "io") == 0) {
      FILE *file = fopen("c:\\users\\android1\\desktop\\test\\audio.pcm", "rb");
      FILE *result = fopen("c:\\users\\android1\\desktop\\test\\left.pcm", "wb");
      int sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
      int nb_samples = 1024;
      int channels = 2;

      AVFrame *frame;
      frame = av_frame_alloc();
      if (!av_frame_get_buffer(frame, 0)) {
        LOGE("get buffer failed\n");
      }
      av_frame_make_writable(frame);
      LOGD("Origin frame writable: %d\n", av_frame_is_writable(frame));
      for (int i = 0; i < 10; ++i) {
        AVFrame *f = frame;
        LOGD("Writable: %d\n", av_frame_is_writable(f));
        read_pcm(file, f, 44100, nb_samples, channels, i, AV_SAMPLE_FMT_FLTP);
        fwrite(f->data[0], sample_size, nb_samples, result);
        av_frame_unref(f);
      }

      fclose(result);
      fclose(file);
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