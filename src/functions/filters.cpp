//
// Created by Infinity on 4/21/22.
//

#include "filters.h"
#include "util.h"
#include "../filter/video_filter.h"
#include "../utils/log.h"
#include "../utils/io.h"
#include "../filter/audio_filter.h"

extern "C" {
#include <libavutil/frame.h>
}


int filter_blur(const char *input, const char *output) {
    FILE *file = fopen(input, "rb");
    FILE *result = fopen(output, "wb");

    AVFrame *frame = av_frame_alloc();

    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = 1920;
    frame->height = 1080;
    av_frame_get_buffer(frame, 0);
    av_frame_make_writable(frame);

    read_yuv(file, frame, frame->width, frame->height, 1, (AVPixelFormat) frame->format);

    VideoFilter filter;
    VideoConfig inConfig((AVPixelFormat) frame->format, frame->width, frame->height);
    VideoConfig outConfig((AVPixelFormat) frame->format, frame->width, frame->height);
    filter.create(
            "gblur=sigma=20:steps=6[blur];[blur]drawtext=fontsize=52:fontcolor=white:text='title':x=w/2-text_w/2:y=h/2-text_h/2",
            &inConfig, &outConfig);

    filter.dumpGraph();


    AVFrame *dest = av_frame_alloc();

    dest->format = AV_PIX_FMT_YUV420P;
    dest->width = 1920;
    dest->height = 1080;
    av_frame_get_buffer(dest, 0);
    av_frame_make_writable(dest);
    int ret = filter.filter(frame, dest);
    if (ret < 0) {
        LOGE("unable to filter frame\n");
        return -1;
    }
    filter.destroy();

    for (int i = 0; i < dest->height; i++) {
        fwrite(dest->data[0] + dest->linesize[0] * i, 1, dest->linesize[0], result);
    }
    for (int i = 0; i < dest->height / 2; i++) {
        fwrite(dest->data[1] + dest->linesize[1] * i, 1, dest->linesize[1], result);
    }
    for (int i = 0; i < dest->height / 2; i++) {
        fwrite(dest->data[2] + dest->linesize[2] * i, 1, dest->linesize[2], result);
    }

    fclose(file);
    fclose(result);
    return 0;
}

int filter_mix(const char * input1, const char * input2, const char * output) {
    FILE *fileA = fopen(input1, "rb");
    FILE *fileB = fopen(input2, "rb");
    FILE *result = fopen(output, "wb");

//            AudioMixFilter filter(AV_CH_LAYOUT_STEREO, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_FLTP,
//                                  44100, 44100, 1.6, 0.2);
//            filter.init();

    AudioFilter filter;

    AudioConfig config{AV_SAMPLE_FMT_FLTP,
                       44100,
                       AV_CH_LAYOUT_STEREO,
                       (AVRational) {1, 2}};

    filter.create("[in1]volume=volume=1.6[out1];[in2]volume=volume=0.4[out2];[out1][out2]amix[out]",
                  &config,
                  &config,
                  &config);

    filter.dumpGraph();

    AVFrame *frameA = av_frame_alloc();
    AVFrame *frameB = av_frame_alloc();
    for (int i = 1; i < 100; ++i) {
        frameA->format = AV_SAMPLE_FMT_FLTP;
        frameA->sample_rate = 44100;
        frameA->nb_samples = 1024;
        frameA->channel_layout = AV_CH_LAYOUT_STEREO;
        av_frame_get_buffer(frameA, 0);
        av_frame_make_writable(frameA);

        frameB->format = AV_SAMPLE_FMT_FLTP;
        frameB->sample_rate = 44100;
        frameB->nb_samples = 1024;
        frameB->channel_layout = AV_CH_LAYOUT_STEREO;
        av_frame_get_buffer(frameB, 0);
        av_frame_make_writable(frameB);

        read_pcm(fileA, frameA, frameA->nb_samples, 2, i, (AVSampleFormat) frameA->format);
        read_pcm(fileB, frameB, frameB->nb_samples, 2, i, (AVSampleFormat) frameB->format);

        filter.filter(frameA, frameB);

        int sample_size = av_get_bytes_per_sample((AVSampleFormat) frameA->format);

        for (int j = 0; j < frameA->nb_samples; ++j) {
            for (int k = 0; k < frameA->channels; ++k) {
                fwrite(frameA->data[k] + j * sample_size, sample_size, 1, result);
            }
        }
    }
    av_frame_free(&frameA);
    av_frame_free(&frameB);

    filter.destroy();

    fclose(fileA);
    fclose(fileB);
    fclose(result);

    return 0;
}