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
    int sourceALayout = AV_CH_LAYOUT_STEREO;
    int sourceBLayout = AV_CH_LAYOUT_STEREO;
    int sourceAFormat = AV_SAMPLE_FMT_NONE;
    int sourceBFormat = AV_SAMPLE_FMT_NONE;
    int sourceARate = 0;
    int sourceBRate = 0;
    float sourceAVolume = 1.0;
    float sourceBVolume = 1.0;
public:
    AudioMixFilter(int sourceALayout, int sourceBLayout,
            int sourceAFormat,
            int sourceBFormat,
            int sourceARate,
            int sourceBRate,
            float volumeA,
            float volumeB);

    int init();

    int filter(AVFrame *sourceA, AVFrame *sourceB, AVFrame* result);

    void destroy();
};

int mix_filter(AVFrame *frameA, AVFrame *frameB);

#endif //VIDEOBOX_MIX_FILTER_H
