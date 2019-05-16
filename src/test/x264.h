//
// Created by wzjing on 2019/4/30.
//

#ifndef VIDEOBOX_X264_
#define VIDEOBOX_X264_

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include "../utils/log.h"

extern "C" {
#include <x264.h>
}

int x264_encode(const char *source, const char *result);

#endif //VIDEOBOX_X264_
