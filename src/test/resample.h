//
// Created by android1 on 2019/4/28.
//

#ifndef VIDEOBOX_RESAMPLE_H
#define VIDEOBOX_RESAMPLE_H

#include "resample.h"
#include <cstdio>
#include "../utils/log.h"
#include "../utils/io.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

int resample(const char *source_a, const char *source_b, const char *result);

#endif //VIDEOBOX_RESAMPLE_H
