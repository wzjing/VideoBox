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
    AVCodecContext *codec_ctx = nullptr;
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

    AVDictionaryEntry *tag = nullptr;

    LOGD("metadata:\n");
    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        printf("\t%s=%s\n", tag->key, tag->value);
    LOGD("\n");

    int have_video = 0;
    int video_index = 0;
    for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            have_video = 1;
            video_index = i;
            break;
        }
    }
    if (!have_video) {
        LOGE("unable to read video from h264 file: %s\n", input_filename);
        return -1;
    }


    AVStream *stream = fmt_ctx->streams[video_index];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);

    if (!codec_ctx) {
        LOGE("code context alloc error\n");
        return -1;
    }

    avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    avcodec_open2(codec_ctx, codec, nullptr);

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.size = 0;
    pkt.data = nullptr;

    AVFrame *frame = av_frame_alloc();

    logContext(codec_ctx, "VideoCodecContext", 1);
    logStream(stream, "VideoStream", 1);

    while (av_read_frame(fmt_ctx, &pkt) == 0) {
        AVPacket packet = pkt;
        if (pkt.stream_index == video_index) {
            logPacket(&packet, "Packet");
            do {
                ret = decode_packet(codec_ctx, frame, &packet, [codec_ctx](AVFrame *f) -> void {
                    char index[8];
                    snprintf(index, 8, "%3d", codec_ctx->frame_number);
                    logFrame(f, index, 1);
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
