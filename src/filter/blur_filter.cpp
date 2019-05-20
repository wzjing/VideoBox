//
// Created by android1 on 2019/5/14.
//

#include "blur_filter.h"

int blur_filter(AVFrame *frame) {
    AVFilterGraph *graph = nullptr;
    AVFilterContext *bufferContext = nullptr;
    AVFilterContext *blurContext = nullptr;
    AVFilterContext *textContext = nullptr;
    AVFilterContext *sinkContext = nullptr;
    const AVFilter *buffer = nullptr;
    const AVFilter *blur = nullptr;
    const AVFilter *text = nullptr;
    const AVFilter *sink = nullptr;
    int ret = 0;

    graph = avfilter_graph_alloc();
    if (!graph) {
        LOGE("Filter Graph error\n");
        return -1;
    }

    create_filter("buffer", "src", buffer, bufferContext, graph,
                  "video_size=1920x1080:pix_fmt=0:time_base=1/30:pixel_aspect=16/9");
    create_filter("gblur", "blur", blur, blurContext, graph, "sigma=30:steps=6");
    create_filter("drawtext", "text", text, textContext, graph,
                  "fontsize=50:fontcolor=white:text='Subtitle':x=w/2-tw/2:y=h/2-th/2");
    create_filter("buffersink", "sink", sink, sinkContext, graph, "");

    avfilter_link(bufferContext, 0, blurContext, 0);
    avfilter_link(blurContext, 0, textContext, 0);
    avfilter_link(textContext, 0, sinkContext, 0);

    ret = avfilter_graph_config(graph, nullptr);
    if (ret < 0) {
        LOGE("unable to configure filter graph: %s\n", av_err2str(ret));
        return -1;
    }

    printf("Graph: %s\n", avfilter_graph_dump(graph, nullptr));

    ret = av_buffersrc_add_frame(bufferContext, frame);
    if (ret < 0) {
        av_frame_unref(frame);
        LOGE("unable to add frame to buffer context");
        return -1;
    }
//    av_frame_unref(frame);
    if (av_buffersink_get_frame(sinkContext, frame) >= 0) {
        LOGD("got result\n");
    } else {
        LOGE("unable to filter frame: %s\n", av_err2str(ret));
        return -1;
    }

    avfilter_graph_free(&graph);
//    av_frame_free(&frame);
    return 0;
}

BlurFilter::BlurFilter(int width, int height, int pix_fmt, float sigma, int steps, const char *title) :
        width(width), height(height), pix_fmt(pix_fmt), sigma(sigma), steps(steps), title(title) {
}

int BlurFilter::init() {
    AVFilterContext *blurContext = nullptr;
    AVFilterContext *textContext = nullptr;
    const AVFilter *buffer = nullptr;
    const AVFilter *blur = nullptr;
    const AVFilter *text = nullptr;
    const AVFilter *sink = nullptr;
    int ret = 0;

    graph = avfilter_graph_alloc();
    if (!graph) {
        LOGE("Filter Graph error\n");
        return -1;
    }
    char args[512];

    snprintf(args, 512, "video_size=%dx%d:pix_fmt=%d:time_base=1/30:pixel_aspect=16/9",
             width, height, pix_fmt);
    create_filter("buffer", "src", buffer, bufferContext, graph, args);
    snprintf(args, 512, "sigma=%f:steps=%d", sigma, steps);
    create_filter("gblur", "blur", blur, blurContext, graph, args);
    snprintf(args, 512, "fontsize=50:fontfile=simhei:fontcolor=white:text='%s':x=w/2-tw/2:y=h/2-th/2", title);
    create_filter("drawtext", "text", text, textContext, graph, args);
    create_filter("buffersink", "sink", sink, sinkContext, graph, "");

    avfilter_link(bufferContext, 0, blurContext, 0);
    avfilter_link(blurContext, 0, textContext, 0);
    avfilter_link(textContext, 0, sinkContext, 0);

    ret = avfilter_graph_config(graph, nullptr);
    if (ret < 0) {
        LOGE("unable to configure filter graph: %s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

int BlurFilter::filter(AVFrame *source) {
    int ret;
    ret = av_buffersrc_add_frame(bufferContext, source);
    if (ret < 0) {
        av_frame_unref(source);
        LOGE("unable to add frame to buffer context");
        return -1;
    }
//    av_frame_unref(source);
    if (av_buffersink_get_frame(sinkContext, source) >= 0) {
        return 0;
    } else {
        LOGE("unable to filter frame: %s\n", av_err2str(ret));
        return -1;
    }
}

void BlurFilter::destroy() {
    avfilter_graph_free(&graph);
}

void BlurFilter::setConfig(float sigmaValue, int stepsValue) {
    this->sigma = sigmaValue;
    this->steps = stepsValue;
}
