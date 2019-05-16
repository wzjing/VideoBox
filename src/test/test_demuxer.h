#ifndef VIDEOBOX_TEST_DEMUXER_H
#define VIDEOBOX_TEST_DEMUXER_H

#include "test_demuxer.h"
#include "../mux/demux.h"
#include "../codec/decode.h"
#include "../utils/snapshot.h"
#include "../utils/log.h"
#include <cstdio>

extern "C" {
#include <libswscale/swscale.h>
}

int test_demuxer(const char *input_filename, const char *output_video_name, const char *output_audio_name);


#endif //VIDEOBOX_TEST_DEMUXER_H
