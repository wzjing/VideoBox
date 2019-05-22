#include "video_filter.h"

#include "../utils/log.h"


int VideoFilter::create(const char *filter_descr) {
    this->description = filter_descr;
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVRational time_base = {1, 1};
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             1920, 1080, AV_PIX_FMT_YUV420P,
             time_base.num, time_base.den,
             16, 9);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        LOGE("Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
    if (ret < 0) {
        LOGE("Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LOGE("Cannot set output pixel format\n");
        goto end;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &inputs, &outputs, nullptr)) < 0) {
        LOGD("Error: unable to parse filter description:%s\n\t%s\n", filter_descr, av_err2str(ret));
        goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0) {
        LOGD("Error: unable to configure filter graph:\n\t%s\n", av_err2str(ret));
        goto end;
    }

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

AVFilterContext *VideoFilter::getInputCtx() {
    return buffersrc_ctx;
}

AVFilterContext *VideoFilter::getOutputCtx() {
    return buffersink_ctx;
}

void VideoFilter::dumpGraph() {
    LOGD("Video Filer(%s):\n%s\n", this->description, avfilter_graph_dump(filter_graph, nullptr));
}

void VideoFilter::destroy() {
    if (filter_graph)
        avfilter_graph_free(&filter_graph);
}
