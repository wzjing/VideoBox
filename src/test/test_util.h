//
// Created by android1 on 2019/5/10.
//

#ifndef VIDEOBOX_TEST_UTIL_H
#define VIDEOBOX_TEST_UTIL_H

#include <cstdio>
#include "../utils/log.h"
#include "../utils/io.h"
extern "C" {
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
}

int x264_test_io(char *input_filename, char *dest_filename);

#endif //VIDEOBOX_TEST_UTIL_H
