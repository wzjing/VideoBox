//
// Created by wzjing on 2019/5/14.
//

#include "filter_common.h"

int create_filter(const char *filter_name, const char *instance_name, const AVFilter *&filter,
                  AVFilterContext *&filter_ctx, AVFilterGraph *&graph, const char *opt) {
    filter = avfilter_get_by_name(filter_name);
    if (!filter) {
        LOGE("unable to find filter: %s\n", filter_name);
        return -1;
    }

    filter_ctx = avfilter_graph_alloc_filter(graph, filter, instance_name);
    if (!filter_ctx) {
        LOGE("unable to create filter context\n");
        return -1;
    }

    int ret = avfilter_init_str(filter_ctx, opt);
    if (ret < 0) {
        LOGE("unable to init filter with options: %s\n", av_err2str(ret));
        return -1;
    }

    return 0;
}
