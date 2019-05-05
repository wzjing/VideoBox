//
// Created by wzjing on 2019/4/30.
//

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include "x264_encode.h"
#include "utils/log.h"

extern "C" {
#include <x264.h>
}

int encode(const char *source, const char *result) {
  int ret;
  FILE *source_file = fopen(source, "rb");
  FILE *result_file = fopen(result, "wb");

  int width = 1920, height = 1080;

  int nb_nal = 0;
  x264_nal_t *nals = nullptr;
  x264_t *handle = nullptr;
  auto *in_pic = (x264_picture_t *) malloc(sizeof(x264_picture_t));
  auto *out_pic = (x264_picture_t *) malloc(sizeof(x264_picture_t));
  auto *param = (x264_param_t *) malloc(sizeof(x264_param_t));

  x264_param_default(param);
  param->i_width = width;
  param->i_height = height;
  param->i_csp = X264_CSP_I420;

  x264_param_apply_profile(param, x264_profile_names[5]);

  handle = x264_encoder_open(param);
  x264_picture_init(out_pic);
  x264_picture_alloc(in_pic, param->i_csp, param->i_width, param->i_height);

  int nb_frames = 180;

  for (int i = 0; i < nb_frames; ++i) {
    fread(in_pic->img.plane[0], width * height, 1, source_file);
    fread(in_pic->img.plane[1], width * height / 4, 1, source_file);
    fread(in_pic->img.plane[2], width * height / 4, 1, source_file);
    in_pic->i_pts = i;

    ret = x264_encoder_encode(handle, &nals, &nb_nal, in_pic, out_pic);
    if (ret < 0) {
      LOGE("x264: Error while encoding.\n");
      return 1;
    }

    LOGD("x264: encode frame %d pts%6ld dts%6ld\n", i, out_pic->i_pts, out_pic->i_dts);
    for (int j = 0; j < nb_nal; ++j) {
      fwrite(nals[j].p_payload, 1, nals[j].i_payload, result_file);
    }
  }

  for (int i = 0;; ++i) {
    ret = x264_encoder_encode(handle, &nals, &nb_nal, nullptr, out_pic);
    if (ret == 0) {
      break;
    }

    LOGD("x264: flush frame %d\n", i);
    for (int j = 0; j < nb_nal; ++j) {
      fwrite(nals[j].p_payload, 1, nals[j].i_payload, result_file);
    }
  }

  x264_picture_clean(in_pic);
  x264_encoder_close(handle);

  free(in_pic);
  free(out_pic);
  free(param);

  fclose(source_file);
  fclose(result_file);

  return 0;
}
