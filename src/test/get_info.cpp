#include "get_info.h"
#include "../utils/log.h"
#include "../codec/decode.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int get_info(const char *input_filename) {
    int ret = 0;
    AVFormatContext *fmt_ctx = nullptr;
    LOGD("filename: %s\n", input_filename);
    ret = avformat_open_input(&fmt_ctx, input_filename, nullptr, nullptr);
    if (ret < 0) {
        LOGE("Could not open source file %s %s\n", input_filename, av_err2str(ret));
        return -1;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGE("Could not find stream information\n");
        return -1;
    }

    logMetadata(fmt_ctx->metadata, "AVFormatContext");

    int video_index = -1;
    int audio_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_index = i;
        }
    }
    if (video_index == -1) {
        LOGW("no track found: %s\n", input_filename);
    }

    if (audio_index == -1) {
        LOGW("no video track found: %s\n", input_filename);
    }


    AVStream *videoStream = fmt_ctx->streams[video_index];
    AVStream *audioStream = fmt_ctx->streams[audio_index];
    AVCodec *videoCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    AVCodec *audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    AVCodecContext *videoContext = avcodec_alloc_context3(videoCodec);
    AVCodecContext *audioContext = avcodec_alloc_context3(audioCodec);

    avcodec_parameters_to_context(videoContext, videoStream->codecpar);
    avcodec_open2(videoContext, videoCodec, nullptr);

    avcodec_parameters_to_context(audioContext, audioStream->codecpar);
    avcodec_open2(audioContext, audioCodec, nullptr);

    AVPacket pkt;
    av_init_packet(&pkt);

    AVFrame *frame = av_frame_alloc();

    logContext(videoContext, "VideoCodecContext", 1);
    logStream(videoStream, "VideoStream", 1);
    logMetadata(videoStream->metadata, "VideoStream");

    while (av_read_frame(fmt_ctx, &pkt) == 0) {
        AVPacket packet = pkt;
        if (pkt.stream_index == video_index) {
//            logPacket(&packet, "VIDEO");
            do {
                ret = decode_packet(videoContext, frame, &packet, [&videoContext](AVFrame *vFrame) -> void {
                    char index[8];
                    snprintf(index, 8, "%3d", videoContext->frame_number);
//                    logFrame(vFrame, index, 1);
                });
            } while (ret);
        } else {
            logPacket(&packet, "AUDIO");
            do {
                ret = decode_packet(audioContext, frame, &packet, [&audioContext](AVFrame *aFrame) -> void {
                    char index[8];
                    snprintf(index, 8, "%3d", audioContext->frame_number);
                    logFrame(aFrame, index, 0);
                });
            } while (ret);
        }
        av_packet_unref(&packet);
    }


    av_frame_free(&frame);
    av_packet_unref(&pkt);
    avformat_free_context(fmt_ctx);

    return 0;
}
