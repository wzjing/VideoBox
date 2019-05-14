//
// Created by android1 on 2019/5/14.
//

#ifndef VIDEOBOX_BLUR_FILTER_H
#define VIDEOBOX_BLUR_FILTER_H

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}

int blur_filter(AVFrame* frame);

#endif //VIDEOBOX_BLUR_FILTER_H
