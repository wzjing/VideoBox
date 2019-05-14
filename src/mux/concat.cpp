//
// Created by android1 on 2019/4/22.
//

#include "concat.h"
#include "../utils/log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
}

int error(const char *message) {
    LOGE("error: file %s line %d\n\t\033[31m%s\033[0m\n", __FILE__, __LINE__, message);
    return -1;
}

void check(int ret, const char *message) {
    if (ret != 0) {
        error(av_err2str(ret));
        exit(1);
    }
}

int concat(const char *input_filename, const char *input_filename1, const char *output_filename) {
    int ret = 0;
    AVFormatContext *iFormatContext = nullptr;
    AVFormatContext *iFormatContext1 = nullptr;
    AVFormatContext *oFormatContext = nullptr;
    AVStream *iStream = nullptr;
    AVStream *iaStream = nullptr;
    AVStream *iStream1 = nullptr;
    AVStream *oStream = nullptr;
    AVStream *oaStream = nullptr;
    AVCodecContext *codecContext;
    AVCodec *codec = nullptr;
    AVPacket *packet;

    ret = avformat_open_input(&iFormatContext, input_filename, nullptr, nullptr);
    check(ret, "input format error");

    ret = avformat_open_input(&iFormatContext1, input_filename1, nullptr, nullptr);
    check(ret, "input format1 error");


    ret = avformat_alloc_output_context2(&oFormatContext, nullptr, nullptr, output_filename);
    check(ret, "output format error");

    for (int i = 0; i < iFormatContext->nb_streams; ++i) {
        if (iFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGE("id: %d\n", i);
            iStream = iFormatContext->streams[i];
            break;
        } else if (iFormatContext1->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            iaStream = iFormatContext1->streams[i];
        }
    }
    if (!iStream) return error("no input video stream");
    if (!iaStream) return error("no input video stream");

    for (int i = 0; i < iFormatContext1->nb_streams; ++i) {
        if (iFormatContext1->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGE("id1: %d\n", i);
            iStream1 = iFormatContext1->streams[i];
            break;
        }
    }
    if (!iStream1) return error("no input video stream1");

    codec = avcodec_find_encoder(iStream->codecpar->codec_id);

    oStream = avformat_new_stream(oFormatContext, codec);
    if (!oStream) return error("output stream error");
    oStream->id = 0;

    codecContext = avcodec_alloc_context3(codec);
    ret = avcodec_parameters_to_context(codecContext, iStream->codecpar);
    check(ret, "Stream to Context");
    codecContext->time_base = AVRational{1, 30};
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    oStream->time_base = codecContext->time_base;
    ret = avcodec_open2(codecContext, codec, nullptr);
    check(ret, "open Context");
    ret = avcodec_parameters_from_context(oStream->codecpar, codecContext);
    check(ret, "Context to Stream");

    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", output_filename);
        ret = avio_open(&oFormatContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(oFormatContext, nullptr);
    check(ret, "write header error");

    av_dump_format(oFormatContext, 0, output_filename, 1);

    LOGD("FrameRate:%d/%d\n"
         "Codec: %s\n"
         "BitRate: %ld\n"
         "IStream: %d/%d\n"
         "OStream: %d/%d\n",
         codecContext->framerate.num,
         codecContext->framerate.den,
         avcodec_get_name(codecContext->codec_id),
         codecContext->bit_rate,
         iStream->time_base.num,
         iStream->time_base.den,
         oStream->time_base.num,
         oStream->time_base.den);


    packet = av_packet_alloc();
    if (!packet) return error("packet error");

    uint64_t last_pts = 0;
    uint64_t last_dts = 0;

    do {
        ret = av_read_frame(iFormatContext, packet);
        if (ret != 0) {
            LOGE("RET: %s\n", av_err2str(ret));
            break;
        }
        if (packet->stream_index == iStream->index) {
            packet->stream_index = oStream->index;
            av_packet_rescale_ts(packet, iStream->time_base, oStream->time_base);
            last_pts = packet->pts + packet->duration;
            last_dts = packet->dts + packet->duration;
            LOGD("packet: %d, %ld, %ld\n", packet->stream_index, packet->dts, packet->pts);
            av_interleaved_write_frame(oFormatContext, packet);
        }
    } while (true);

    do {
        ret = av_read_frame(iFormatContext1, packet);
        if (ret != 0) {
            LOGE("RET: %s\n", av_err2str(ret));
            break;
        }
        if (packet->stream_index == iStream1->index) {
            packet->stream_index = oStream->index;
            av_packet_rescale_ts(packet, iStream1->time_base, oStream->time_base);
            packet->pts += last_pts;
            packet->dts += last_dts;
            LOGD("packet: %d, %ld, %ld\n", packet->stream_index, packet->dts, packet->pts);
            av_interleaved_write_frame(oFormatContext, packet);
        }
    } while (true);

    av_write_trailer(oFormatContext);

    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&oFormatContext->pb);
    }

    av_packet_free(&packet);

    avformat_free_context(iFormatContext);
    avformat_free_context(oFormatContext);
    return 0;
}
