//
// Created by android1 on 2019/5/14.
//


#include "mix_filter.h"

int mix_filter(AVFrame *frameA, AVFrame *frameB) {
    AVFilterGraph *filter_graph;
    AVFilterContext *buffer_a_ctx, *buffer_b_ctx, *volume_a_ctx, *volume_b_ctx, *mix_ctx, *format_ctx, *sink_ctx;
    const AVFilter *buffer_a, *buffer_b, *volume_a, *volume_b, *mix, *format, *sink;
    int ret;

    char args[512];
    char ch_layout[64];

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        LOGE("unable to create filter graph\n");
        return -1;
    }
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout),
                                 av_get_channel_layout_nb_channels(frameA->channel_layout), frameA->channel_layout);
    LOGE("Channel_layout: %s\n", ch_layout);

    // source a filter
    snprintf(args, 512, "channel_layout=%s:sample_fmt=%d:sample_rate=%d:time_base=%d/%d",
             ch_layout, frameA->format, frameA->sample_rate, 1, frameA->sample_rate);
    create_filter("abuffer", "src0", buffer_a, buffer_a_ctx, filter_graph, args);

    // source b filter
    snprintf(args, 512, "channel_layout=%s:sample_fmt=%d:sample_rate=%d:time_base=%d/%d",
             ch_layout, frameB->format, frameB->sample_rate, 1, frameB->sample_rate);
    create_filter("abuffer", "src1", buffer_b, buffer_b_ctx, filter_graph, args);

    // volume a filter
    create_filter("volume", "volume0", volume_a, volume_a_ctx, filter_graph, "volume=1.2");

    // volume b filter
    create_filter("volume", "volume1", volume_b, volume_b_ctx, filter_graph, "volume=0.8");

    // mix filter
    create_filter("amix", "amix", mix, mix_ctx, filter_graph, "");

    // format filter
//    snprintf(args, 512, "channel_layout=%s:sample_fmt=%d:sample_rate=%d:time_base=%d/%d",
//             ch_layout, frameA->format, frameA->sample_rate, 1, frameA->sample_rate);
//    create_filter("aformat", "aformat", format, format_ctx, filter_graph, "");

    // sink filter
    create_filter("abuffersink", "sink", sink, sink_ctx, filter_graph, nullptr);

    avfilter_link(buffer_a_ctx, 0, volume_a_ctx, 0);
    avfilter_link(buffer_b_ctx, 0, volume_b_ctx, 0);
    avfilter_link(volume_a_ctx, 0, mix_ctx, 0);
    avfilter_link(volume_b_ctx, 0, mix_ctx, 1);
//    avfilter_link(mix_ctx, 0, format_ctx, 0);
    avfilter_link(mix_ctx, 0, sink_ctx, 0);

    ret = avfilter_graph_config(filter_graph, nullptr);
    if (ret < 0) {
        LOGE("unable to configure filter graph: %s\n", av_err2str(ret));
        return -1;
    }
    printf("Graph: %s\n", avfilter_graph_dump(filter_graph, nullptr));

    // start mix filter
    ret = av_buffersrc_add_frame(buffer_a_ctx, frameA);
    if (ret < 0) {
        av_frame_unref(frameA);
        LOGE("unable to add frame to buffer a context");
        return -1;
    }

    ret = av_buffersrc_add_frame(buffer_b_ctx, frameB);
    if (ret < 0) {
        av_frame_unref(frameB);
        LOGE("unable to add frame to buffer b context");
        return -1;
    }
    av_frame_unref(frameA);
    LOGD("start receive\n");
    ret = av_buffersink_get_frame(sink_ctx, frameA);
    if (ret >= 0) {
        LOGD("got mix frame\n");
    } else {
        LOGE("unable to filter frame: %s\n", av_err2str(ret));
        return -1;
    }

    avfilter_graph_free(&filter_graph);
    return 0;

}


AudioMixFilter::AudioMixFilter(int sourceALayout, int sourceBLayout, int sourceAFormat, int sourceBFormat,
                               int sourceARate, int sourceBRate, float volumeA, float volumeB) :
        sourceALayout(sourceALayout),
        sourceBLayout(sourceBLayout),
        sourceAFormat(sourceAFormat),
        sourceBFormat(sourceBFormat),
        sourceARate(sourceARate),
        sourceBRate(sourceBRate),
        sourceAVolume(volumeA),
        sourceBVolume(volumeB) {

}

int AudioMixFilter::init() {
    graph = avfilter_graph_alloc();
    if (!graph) {
        LOGE("unable to create filter graph\n");
        return -1;
    }

    AVFilterContext *formatAContext;
    AVFilterContext *formatBContext;
    AVFilterContext *volumeAContext;
    AVFilterContext *volumeBContext;
    AVFilterContext *mixContext;
    const AVFilter *formatA;
    const AVFilter *formatB;
    const AVFilter *bufferA;
    const AVFilter *bufferB;
    const AVFilter *volume_a;
    const AVFilter *volume_b;
    const AVFilter *mix;
    const AVFilter *sink;

    char args[512];
    char aLayout[64];
    char bLayout[64];
    av_get_channel_layout_string(aLayout, sizeof(aLayout),
                                 av_get_channel_layout_nb_channels(sourceALayout), sourceALayout);
    av_get_channel_layout_string(bLayout, sizeof(bLayout),
                                 av_get_channel_layout_nb_channels(sourceBLayout), sourceBLayout);

    const char *aFormat = av_get_sample_fmt_name((AVSampleFormat) sourceAFormat);
    const char *bFormat = av_get_sample_fmt_name((AVSampleFormat) sourceBFormat);

    int aChannels = av_get_channel_layout_nb_channels(sourceALayout);
    int bChannels = av_get_channel_layout_nb_channels(sourceBLayout);

    // source a filter
    snprintf(args, 512, "channel_layout=%s:sample_fmt=%s:sample_rate=%d:time_base=%d/%d:channels=%d",
             aLayout, aFormat, sourceARate, 1, sourceARate, aChannels);
    create_filter("abuffer", "src_a", bufferA, bufferAContext, graph, args);

    // source b filter
    snprintf(args, 512, "channel_layout=%s:sample_fmt=%s:sample_rate=%d:time_base=%d/%d:channels=%d",
             bLayout, bFormat, sourceBRate, 1, sourceBRate, bChannels);
    create_filter("abuffer", "src_b", bufferB, bufferBContext, graph, args);

    // aformat a filter
    snprintf(args, 512, "sample_fmts=%s:sample_rates=%d:channel_layouts=%s",
             aFormat, sourceARate, aLayout);
    create_filter("aformat", "format_a", formatA, formatAContext, graph, args);

    // aformat b filter
    snprintf(args, 512, "sample_fmts=%s:sample_rates=%d:channel_layouts=%s",
             bFormat, sourceBRate, bLayout);
    create_filter("aformat", "format_b", formatB, formatBContext, graph, args);

    // volume a filter
    snprintf(args, 512, "volume=%f", sourceAVolume);
    create_filter("volume", "volume_a", volume_a, volumeAContext, graph, args);

    // volume b filter
    snprintf(args, 512, "volume=%f", sourceBVolume);
    create_filter("volume", "volume_b", volume_b, volumeBContext, graph, args);

    // mix filter
    create_filter("amix", "amix", mix, mixContext, graph, "");

    // sink filter
    create_filter("abuffersink", "sink", sink, sinkContext, graph, "");

    avfilter_link(bufferAContext, 0, formatAContext, 0);
    avfilter_link(bufferBContext, 0, formatBContext, 0);
    avfilter_link(formatAContext, 0, volumeAContext, 0);
    avfilter_link(formatBContext, 0, volumeBContext, 0);
    avfilter_link(volumeAContext, 0, mixContext, 0);
    avfilter_link(volumeBContext, 0, mixContext, 1);
    avfilter_link(mixContext, 0, sinkContext, 0);
    int ret = 0;
    ret = avfilter_graph_config(graph, nullptr);
    if (ret < 0) {
        LOGE("unable to configure filter graph: %s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

int AudioMixFilter::filter(AVFrame *sourceA, AVFrame *sourceB, AVFrame* result) {

    int ret;

    ret = av_buffersrc_add_frame(bufferAContext, sourceA);
    if (ret < 0) {
        av_frame_unref(sourceA);
        LOGE("unable to add frame to buffer a context\n");
        return -1;
    }

    ret = av_buffersrc_add_frame(bufferBContext, sourceB);
    if (ret < 0) {
        av_frame_unref(sourceB);
        LOGE("unable to add frame to buffer b context\n");
        return -1;
    }
    av_frame_unref(sourceA);
    ret = av_buffersink_get_frame(sinkContext, result);
    if (ret == 0) {
        return 0;
    } else if (ret == AVERROR(EAGAIN)) {
        LOGW("mix unavailable\n");
        return ret;
    } else if (ret == AVERROR_EOF) {
        LOGW("mix eof\n");
        return ret;
    } else {
        LOGE("unable to filter frame: %s\n", av_err2str(ret));
        return -1;
    }
}

void AudioMixFilter::destroy() {
    avfilter_graph_free(&graph);
}
