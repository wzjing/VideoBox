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

    create_filter("buffer", "src", buffer, bufferContext, graph, "video_size=1920x1080:pix_fmt=0:time_base=1/30:pixel_aspect=16/9");
    create_filter("gblur", "blur", blur, blurContext, graph, "sigma=30:steps=6");
    create_filter("drawtext", "text", text, textContext, graph, "fontsize=50:text='Subtitle':x=500:y=500");
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
