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
#include "concat.h"
#include "filter/blur_filter.h"
#include "filter/mix_filter.h"
#include "mux_title.h"

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
        else if (check("concat")) return concat(argv[2], &argv[3], 2);
        else if (check("mux_title")) return mux_title(argv[2], argv[3]);
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

            AudioMixFilter filter(AV_CH_LAYOUT_STEREO, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_FLTP,
                                  44100, 44100, 1.6, 0.2);
            filter.init();

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

                filter.filter(frameA, frameB, nullptr);

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
    }

    fprintf(stderr, "invalid command: ");
    for (int j = 0; j < argc; ++j) {
        printf("%s ", argv[j]);
    }
    printf("\n");
    return -1;
}