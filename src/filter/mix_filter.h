//
// Created by android1 on 2019/5/14.
//

#ifndef VIDEOBOX_MIX_FILTER_H
#define VIDEOBOX_MIX_FILTER_H

#include "filter_common.h"

class AudioMixFilter {
private:
    AVFilterGraph *graph = nullptr;
    AVFilterContext *bufferAContext = nullptr;
    AVFilterContext *bufferBContext = nullptr;
    AVFilterContext *sinkContext = nullptr;
    int channel_layout = AV_CH_LAYOUT_STEREO;
    int sample_fmt = AV_SAMPLE_FMT_NONE;
    int sample_rate = 0;
    float sourceAVolume = 1.0;
    float sourceBVolume = 1.0;
public:
    AudioMixFilter(int channel_layout, int sample_fmt, int sample_rate, float volumeA, float volumeB);

    int init();

    int filter(AVFrame *sourceA, AVFrame *sourceB);

    void destroy();
};

int mix_filter(AVFrame *frameA, AVFrame *frameB);

#endif //VIDEOBOX_MIX_FILTER_H
