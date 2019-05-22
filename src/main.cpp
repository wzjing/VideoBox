#include <cstdio>
#include <cstring>
#include <assert.h>

#include "codec/decode.h"
#include "utils/log.h"
#include "utils/io.h"
#include "test/test_demuxer.h"
#include "test/mux_encode.h"
#include "test/test_muxer.h"
#include "test/resample.h"
#include "test/filter_audio.h"
#include "test/x264.h"
#include "test/get_info.h"
#include "test/test_util.h"
#include "mux/mux.h"
#include "concat_bgm.h"
#include "filter/blur_filter.h"
#include "filter/mix_filter.h"
#include "filter/video_filter.h"
#include "mux_title.h"
#include "filter/audio_filter.h"
#include "mix_bgm.h"
#include "concat_add_title.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
}

int main(int argc, char *argv[]) {

    auto check = [&argv](const char *command) -> bool {
        return strcmp(argv[1], command) == 0;
    };

    if (argc >= 1) {
        if (check("info")) return get_info(argv[2]);
        else if (check("mux_exp")) return mux_encode(argv[2], argv[3], argv[4]);
        else if (check("muxer")) return test_muxer(argv[2], argv[3], argv[4]);
        else if (check("mux")) return test_mux(argv[2], argv[3], argv[4], 4);
        else if (check("demuxer")) return test_demuxer(argv[2], argv[3], argv[4]);
        else if (check("concat")) return concat(argv[2], argv[3], &argv[4], 4);
        else if (check("concat_add_title")) return concat_add_title(argv[2], &argv[3], 3);
        else if (check("mux_title")) return mux_title(argv[2], argv[3]);
        else if (check("mix_bgm")) return mix_bgm(argv[2], argv[3], argv[4], 1.2);
        else if (check("x264_encode")) return x264_encode(argv[2], argv[3]);
        else if (check("filter_blur")) {
            FILE *file = fopen(argv[2], "rb");
            FILE *result = fopen(argv[3], "wb");

            AVFrame *frame = av_frame_alloc();

            frame->format = AV_PIX_FMT_YUV420P;
            frame->width = 1920;
            frame->height = 1080;
            av_frame_get_buffer(frame, 0);
            av_frame_make_writable(frame);

            read_yuv(file, frame, frame->width, frame->height, 1, (AVPixelFormat) frame->format);

            blur_filter(frame);

            for (int i = 0; i < frame->height; i++) {
                fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->linesize[0], result);
            }
            for (int i = 0; i < frame->height / 2; i++) {
                fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->linesize[1], result);
            }
            for (int i = 0; i < frame->height / 2; i++) {
                fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->linesize[2], result);
            }

            fclose(file);
            fclose(result);
            return 0;
        } else if (check("filter_mix")) {
            FILE *fileA = fopen(argv[2], "rb");
            FILE *fileB = fopen(argv[3], "rb");
            FILE *result = fopen(argv[4], "wb");

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
        } else if (check("test")) {
            int ret = 0;
            VideoFilter filter;
            ret = filter.create("color=black:200x200[c];"
                                "[c]setsar,drawtext=fontsize=30:fontcolor=white:text='Hello':x=50:y=50,split[text][alpha];"
                                "[text][alpha]alphamerge,rotate=angle=-PI/2[rotate];"
                                "[in][rotate]overlay=x=500:y=500[out]");
//            ret = filter.init("movie=/mnt/c/users/android1/desktop/mark.png[mark];[in][mark]overlay=100:100");
//            ret = init_filters("drawgrid=width=100:height=100:thickness=4:color=pink@0.9");
//            ret = filter.init("[in]drawtext=fontsize=40:fontcolor=red:text='Title'[out]");
//            ret = filter.init(
//                    "color=white:200x200[canvas];[canvas]drawtext=fontsize=40:fontcolor=red:text='Title'[text];[in][text]overlay=100:100[out]");
            filter.dumpGraph();
            if (ret < 0) {
                LOGE("init failed: %s\n", av_err2str(ret));
                return ret;
            }

            FILE *data = fopen("/mnt/c/Users/android1/Desktop/video.yuv", "rb");
            for (int i = 0; i < 3; ++i) {
                AVFrame *input = av_frame_alloc();
                input->width = 1920;
                input->height = 1080;
                input->format = AV_PIX_FMT_YUV420P;
                av_frame_get_buffer(input, 0);
                read_yuv(data, input, input->width, input->height, i, (AVPixelFormat) input->format);
                ret = av_buffersrc_add_frame(filter.getInputCtx(), input);
                if (ret < 0) {
                    LOGE("add input error: %s\n", av_err2str(ret));
                    return -1;
                }
                av_frame_unref(input);
                do {
                    AVFrame *output = av_frame_alloc();
                    output->format = AV_PIX_FMT_YUV420P;
                    output->width = 1920;
                    output->height = 1080;
                    ret = av_buffersink_get_frame(filter.getOutputCtx(), output);
                    if (ret == 0) {
                        LOGD("Frame format: %s\n", av_get_pix_fmt_name((AVPixelFormat) output->format));
                        for (int j = 0; input->linesize[j]; ++j) {
                            LOGD("Frame buffer %d: %d\n", j, input->linesize[j]);
                        }
                        char filename[128];
                        snprintf(filename, 128, "/mnt/c/Users/android1/Desktop/filtered%d.yuv", i);
                        save_av_frame(output, filename);
                    }
                    av_frame_free(&output);
                } while (ret == 0);
                av_frame_free(&input);
            }
            return 0;
        }
    }

    fprintf(stderr, "invalid command: ");
    for (int j = 0; j < argc; ++j) {
        printf("%s ", argv[j]);
    }
    printf("\n");
    return -1;
}