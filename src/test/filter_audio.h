//
// Created by android1 on 2019/4/29.
//

#ifndef VIDEOBOX_FILTER_AUDIO_H
#define VIDEOBOX_FILTER_AUDIO_H

//
// Created by android1 on 2019/4/29.
//

#include "filter_audio.h"
#include "../utils/log.h"
#include "../utils/io.h"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
}

int audio_filter(const char *source_a, const char *source_b, const char *result);

#endif //VIDEOBOX_FILTER_AUDIO_H
