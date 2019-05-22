//
// Created by wzjing on 2019/5/19.
//

#ifndef VIDEOBOX_VIDEO_FILTER_H
#define VIDEOBOX_VIDEO_FILTER_H


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

class VideoFilter {
protected:
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;
    const char *description = nullptr;
public:

    VideoFilter() = default;

    int create(const char *filter_descr);

    AVFilterContext *getInputCtx();

    AVFilterContext *getOutputCtx();

    void dumpGraph();

    void destroy();
};


#endif //VIDEOBOX_VIDEO_FILTER_H
