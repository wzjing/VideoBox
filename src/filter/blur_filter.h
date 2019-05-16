//
// Created by android1 on 2019/5/14.
//

#ifndef VIDEOBOX_BLUR_FILTER_H
#define VIDEOBOX_BLUR_FILTER_H

#include "filter_common.h"

class BlurFilter {
private:
    AVFilterGraph *graph = nullptr;
    AVFilterContext *bufferContext = nullptr;
    AVFilterContext *sinkContext = nullptr;
    int width = 0;
    int height = 0;
    int pix_fmt = AV_PIX_FMT_NONE;
    float sigma = 20;
    int steps = 4;
    const char *title = "title";
public:
    BlurFilter(int width, int height, int pix_fmt, float sigma, int steps, const char *title);

    int init();

    int filter(AVFrame *source);

    void destroy();
};

int blur_filter(AVFrame *frame);

#endif //VIDEOBOX_BLUR_FILTER_H
