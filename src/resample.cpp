//
// Created by android1 on 2019/4/28.
//

#include "resample.h"
#include <cstdio>
#include "utils/log.h"
#include "utils/io.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

void resample(const char *source_a, const char *source_b, const char *result) {

  int ret;
  int nb_samples = 1024 * 180;
  long dst_nb_samples;
  int sample_rate = 44100;
  AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;
  int sample_size = av_get_bytes_per_sample((AVSampleFormat) sample_fmt);

  uint8_t **src_data = nullptr, **dst_data = nullptr;
  int *src_linesize = nullptr, *dst_linesize = nullptr;
  av_samples_alloc_array_and_samples(&src_data, src_linesize, 4, nb_samples, sample_fmt, 0);

  FILE *file_a = fopen(source_a, "rb");
  FILE *file_b = fopen(source_b, "rb");
  FILE *result_file = fopen(result, "wb");

  read_pcm_to_raw(file_a, src_data, nb_samples, 2, 0, sample_fmt);
  read_pcm_to_raw(file_b, src_data + 2, nb_samples, 2, 0, sample_fmt);

  SwrContext *swr_ctx;
  swr_ctx = swr_alloc();

  if (!swr_ctx) {
    LOGE("Error alloc context\n");
    exit(1);
  }

  av_opt_set_int(swr_ctx, "in_channel_layout", AV_CH_LAYOUT_4POINT0, 0);
  av_opt_set_int(swr_ctx, "in_sample_rate", sample_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", sample_fmt, 0);

  av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
  av_opt_set_int(swr_ctx, "out_sample_rate", sample_rate, 0);
  av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", sample_fmt, 0);

  if (swr_init(swr_ctx) < 0) {
    LOGE("Error init context\n");
    goto end;
  }

  dst_nb_samples = av_rescale_rnd(nb_samples, sample_rate, sample_rate, AV_ROUND_UP);
  av_samples_alloc_array_and_samples(&dst_data, dst_linesize, 2, dst_nb_samples, sample_fmt, 0);

  ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **) src_data, nb_samples);

  if (ret < 0) {
    LOGE("Error while convert: %s\n", av_err2str(ret));
    goto end;
  }

  LOGD("Dst samples: %ld\n", dst_nb_samples);

  LOGD("Data: %d", dst_data[0][0]);

  for (int i = 0; i < dst_nb_samples; ++i) {
    fwrite(dst_data[0] + i * sample_size, sample_size, 1, result_file);
    fwrite(dst_data[1] + i * sample_size, sample_size, 1, result_file);
  }

  fflush(result_file);

  end:
  fclose(file_a);
  fclose(file_b);
  fclose(result_file);
}
