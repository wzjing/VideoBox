//
// Created by android1 on 2019/5/10.
//

#ifndef VIDEOBOX_TEST_IO_HPP
#define VIDEOBOX_TEST_IO_HPP

#include <cstdio>
#include "../utils/log.h"
#include "../utils/io.h"
extern "C" {
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
}

int test_io(char *input_filename, char *dest_filename) {
  FILE *file = fopen(input_filename, "rb");
  FILE *result = fopen(dest_filename, "wb");
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
    for (int j = 0; j < frame->nb_samples; ++j) {
      for (int ch = 0; ch < frame->channels; ++ch) {
        fwrite(frame->data[ch] + j * sample_size, sample_size, 1, result);
      }
    }
  }

  av_frame_free(&frame);

  fclose(result);
  fclose(file);
}

#endif //VIDEOBOX_TEST_IO_HPP
