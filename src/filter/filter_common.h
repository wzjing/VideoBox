//
// Created by wzjing on 2019/5/14.
//

#ifndef VIDEOBOX_FILTER_COMMON_H
#define VIDEOBOX_FILTER_COMMON_H

#include "../utils/log.h"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}

int create_filter(const char *filter_name, const char *instance_name, const AVFilter *&filter,
                  AVFilterContext *&filter_ctx, AVFilterGraph *&graph,
                  const char * opt);

#endif //VIDEOBOX_FILTER_COMMON_H
