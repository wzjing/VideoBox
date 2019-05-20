#include "filter.h"

int Filter::setDescription(const char *description) {
    this->filter_description = description;
    return 0;
}

int Filter::setInput(AVFrame *frame, const char *pod) {
    if (!pod) pod = "in";
    if (!filterGraph) {
        filterGraph = avfilter_graph_alloc();
    }

    AVFilterContext *bufferContext = nullptr;
    const AVFilter *buffer = avfilter_get_by_name("buffer");
    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width, frame->height, frame->format,
             1, 30,
             frame->width, frame->width);
    avfilter_graph_create_filter(&bufferContext, buffer, pod, args, nullptr, filterGraph);
    AVFilterInOut *input = avfilter_inout_alloc();
    input->name = av_strdup(pod);
    input->filter_ctx = bufferContext;
    input->pad_idx = 0;
    input->next = nullptr;

    addInput(input, bufferContext, pod);
    return 0;
}

int Filter::getOutput(AVFrame *frame, const char *pod) {
    if (!pod) pod = "out";
    if (!filterGraph) {
        filterGraph = avfilter_graph_alloc();
    }

    AVFilterContext *sinkContext = nullptr;
    const AVFilter *sink = avfilter_get_by_name("sink");
    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width, frame->height, frame->format,
             1, 30,
             frame->width, frame->width);
    avfilter_graph_create_filter(&sinkContext, sink, pod, args, nullptr, filterGraph);
    AVFilterInOut *output = avfilter_inout_alloc();
    output->name = av_strdup(pod);
    output->filter_ctx = sinkContext;
    output->pad_idx = 0;
    output->next = nullptr;

    addOutput(output, sinkContext, pod);
    createChain();


    return 0;
}
