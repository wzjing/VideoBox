//
// Created by android1 on 2019/5/14.
//

#ifndef VIDEOBOX_MIX_FILTER_H
#define VIDEOBOX_MIX_FILTER_H

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
}
void mix_filter(AVFrame * input_frame);

#endif //VIDEOBOX_MIX_FILTER_H
