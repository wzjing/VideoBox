//
// Created by wzjing on 2019/5/19.
//

#ifndef VIDEOBOX_FILTER_H
#define VIDEOBOX_FILTER_H

#include <libavcodec/avcodec.h>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

struct AFilterIO {
    AVFilterContext *buffer = nullptr;
    AVFilterInOut *input = nullptr;
    const char *pod = nullptr;
};

struct VFilterIO {
    int width = 0;
    int height =0;
    int format = 0;
    AVRational timebase;
    AVRational aspectRatio;
    const char * pod = nullptr;
};

class Filter {
protected:
    const char *filter_description = nullptr;
    AVFilterGraph *filterGraph = nullptr;
    FilterIO *inputs = nullptr;
    FilterIO *outputs = nullptr;
    int nb_input = 0;
    int chainAvailable = 0;

    FilterIO *findInputByPod();

    FilterIO *findOutputByPod();

    int addInput(AVFilterInOut *input, AVFilterContext *context, const char *pod);

    int addOutput(AVFilterInOut *output, AVFilterContext *context, const char *pod);

    int createChain();

    int clearChain();

public:

    Filter() = default;

    int setDescription(const char *description);

    int setInput(AVCodecContext *codecContext, const char *pod);

    int setOutput(AVCodecContext *codecContext, const char *pod);

    int filter(AVFrame *frame, const char *pod);
};


#endif //VIDEOBOX_FILTER_H
