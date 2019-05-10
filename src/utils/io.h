//
// Created by android1 on 2019/4/26.
//

#ifndef VIDEOBOX_IO_H
#define VIDEOBOX_IO_H

#include "log.h"

#include <cstdint>
#include <cstdio>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>
}

int read_yuv(FILE *file, AVFrame *frame, int width, int height, int index,
              enum AVPixelFormat pix_fmt);

int read_pcm(FILE *file, AVFrame *frame, int nb_samples, int channels, int index,
              enum AVSampleFormat sample_fmt);

void read_pcm_to_raw(FILE *file, uint8_t **data, int nb_samples, int channels, int index,
              enum AVSampleFormat sample_fmt);

#endif //VIDEOBOX_IO_H
