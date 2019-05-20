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

AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;

static int init_filters(const char *filters_descr) {
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVRational time_base = {1, 30};
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             1920, 1080, AV_PIX_FMT_YUV420P,
             time_base.num, time_base.den,
             16, 9);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, nullptr)) < 0) {
        LOGD("Error1: %s\n", av_err2str(ret));
        goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
        LOGD("Error2: %s\n", av_err2str(ret));
        goto end;
    }

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
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
        else if (check("concat_bgm")) return concat_with_bgm(argv[2], argv[3], &argv[4], 2);
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
        } else if (check("test")) {
            int ret = 0;
//            ret = init_filters("color=black:200x200[c];"
//                               "[c]setsar,drawtext=fontsize=30:fontcolor=white:text='Hello':x=50:y=50,split[text][alpha];"
//                               "[text][alpha]alphamerge,rotate=angle=-PI/2[rotate];"
//                               "[in][rotate]overlay=x=500:y=500[out]");
//            ret = init_filters("color=white:200x200[c];[in][c]overlay=5:5");
            ret = init_filters("color=black:255x255[ov];[in][ov]overlay=255:255[out]");
//            ret = init_filters("drawgrid=width=100:height=100:thickness=4:color=pink@0.9");

            LOGD(avfilter_graph_dump(filter_graph, nullptr));
            if (ret < 0) {
                LOGE("init failed\n");
                return ret;
            }
            FILE *data = fopen("/mnt/c/Users/wzjing/Desktop/video.yuv", "rb");
            AVFrame *input = av_frame_alloc();
            input->width = 1920;
            input->height = 1080;
            input->format = AV_PIX_FMT_YUV420P;
            av_frame_get_buffer(input, 0);
            read_yuv(data, input, input->width, input->height, 1, (AVPixelFormat) input->format);
            LOGD("input1: %d\n", input->linesize[1]);
            ret = av_buffersrc_add_frame_flags(buffersrc_ctx, input, AV_BUFFERSRC_FLAG_KEEP_REF);
            if (ret < 0) {
                LOGE("input: %s\n", av_err2str(ret));
                return -1;
            }
            AVFrame *output = av_frame_alloc();
            output->format = AV_PIX_FMT_YUV420P;
            output->width = 1920;
            output->height = 1080;
            ret = av_buffersink_get_frame(buffersink_ctx, output);
            if (ret == 0) {
                LOGD("format: %s\n", av_get_pix_fmt_name((AVPixelFormat) output->format));
                LOGD("output_0: %d\n", output->linesize[0]);
                LOGD("output_1: %d\n", output->linesize[1]);
                return save_av_frame(output, "/mnt/c/Users/wzjing/Desktop/filtered.yuv");
            }
            return -2;
        }
    }

    fprintf(stderr, "invalid command: ");
    for (int j = 0; j < argc; ++j) {
        printf("%s ", argv[j]);
    }
    printf("\n");
    return -1;
}