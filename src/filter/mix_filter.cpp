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

AudioMixFilter::AudioMixFilter(int channel_layout, int sample_fmt, int sample_rate, float volumeA, float volumeB) :
        channel_layout(channel_layout),
        sample_fmt(sample_fmt),
        sample_rate(sample_rate),
        sourceAVolume(volumeA),
        sourceBVolume(volumeB) {
}

int AudioMixFilter::init() {
    graph = avfilter_graph_alloc();
    if (!graph) {
        LOGE("unable to create filter graph\n");
        return -1;
    }

    AVFilterContext *volume_a_ctx, *volume_b_ctx, *mix_ctx;
    const AVFilter *bufferA, *bufferB, *volume_a, *volume_b, *mix, *sink;

    char args[512];
    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout),
                                 av_get_channel_layout_nb_channels(channel_layout), channel_layout);
    LOGD("Channel_layout: %s sample_fmt: %s\n", ch_layout, av_get_sample_fmt_name((AVSampleFormat) sample_fmt));

    // source a filter
    snprintf(args, 512, "channel_layout=%s:sample_fmt=%d:sample_rate=%d:time_base=%d/%d",
             ch_layout, sample_fmt, sample_rate, 1, sample_rate);
    LOGD("args: %s\n", args);
    create_filter("abuffer", "src0", bufferA, bufferAContext, graph, args);

    // source b filter
    create_filter("abuffer", "src1", bufferB, bufferBContext, graph, args);

    // volume a filter
    snprintf(args, 512, "volume=%f", sourceAVolume);
    create_filter("volume", "volume0", volume_a, volume_a_ctx, graph, args);

    // volume b filter
    snprintf(args, 512, "volume=%f", sourceBVolume);
    create_filter("volume", "volume1", volume_b, volume_b_ctx, graph, args);

    // mix filter
    create_filter("amix", "amix", mix, mix_ctx, graph, "");

    // sink filter
    create_filter("abuffersink", "sink", sink, sinkContext, graph, "");

    avfilter_link(bufferAContext, 0, volume_a_ctx, 0);
    avfilter_link(bufferBContext, 0, volume_b_ctx, 0);
    avfilter_link(volume_a_ctx, 0, mix_ctx, 0);
    avfilter_link(volume_b_ctx, 0, mix_ctx, 1);
    avfilter_link(mix_ctx, 0, sinkContext, 0);
    int ret = 0;
    ret = avfilter_graph_config(graph, nullptr);
    if (ret < 0) {
        LOGE("unable to configure filter graph: %s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

int AudioMixFilter::filter(AVFrame *sourceA, AVFrame *sourceB) {
//    AVFilterGraph *filterGraph = avfilter_graph_alloc();
//    if (!filterGraph) {
//        LOGE("unable to create filter graph\n");
//        return -1;
//    }
//
//    AVFilterContext *buffer_a_ctx, *buffer_b_ctx, *volume_a_ctx, *volume_b_ctx, *mix_ctx, *sink_ctx;
//    const AVFilter *bufferA, *bufferB, *volume_a, *volume_b, *mix, *sink;
//
//    char args[512];
//    char ch_layout[64];
//    av_get_channel_layout_string(ch_layout, sizeof(ch_layout),
//                                 av_get_channel_layout_nb_channels(channel_layout), channel_layout);
//    LOGD("Channel_layout: %s sample_fmt: %s\n", ch_layout, av_get_sample_fmt_name((AVSampleFormat) sample_fmt));
//
//    // source a filter
//    snprintf(args, 512, "channel_layout=%s:sample_fmt=%d:sample_rate=%d:time_base=%d/%d",
//             ch_layout, sourceA->format, sourceA->sample_rate, 1, sourceA->sample_rate);
//    LOGD("args: %s\n", args);
//    create_filter("abuffer", "src0", bufferA, buffer_a_ctx, filterGraph, args);
//
//    // source b filter
//    create_filter("abuffer", "src1", bufferB, buffer_b_ctx, filterGraph, args);
//
//    // volume a filter
//    snprintf(args, 512, "volume=%f", sourceAVolume);
//    create_filter("volume", "volume0", volume_a, volume_a_ctx, filterGraph, "volume=1.2");
//
//    // volume b filter
//    snprintf(args, 512, "volume=%f", sourceBVolume);
//    create_filter("volume", "volume1", volume_b, volume_b_ctx, filterGraph, "volume=0.8");
//
//    // mix filter
//    create_filter("amix", "amix", mix, mix_ctx, filterGraph, "");
//
//    // sink filter
//    create_filter("abuffersink", "sink", sink, sink_ctx, filterGraph, "");
//
//    avfilter_link(buffer_a_ctx, 0, volume_a_ctx, 0);
//    avfilter_link(buffer_b_ctx, 0, volume_b_ctx, 0);
//    avfilter_link(volume_a_ctx, 0, mix_ctx, 0);
//    avfilter_link(volume_b_ctx, 0, mix_ctx, 1);
//    avfilter_link(mix_ctx, 0, sink_ctx, 0);

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
    LOGD("start receive\n");
    ret = av_buffersink_get_frame(sinkContext, sourceA);
    if (ret >= 0) {
        LOGD("got mix frame\n");
    } else {
        LOGE("unable to filter frame: %s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

void AudioMixFilter::destroy() {
    avfilter_graph_free(&graph);
}
