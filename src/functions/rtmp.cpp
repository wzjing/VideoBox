//
// Created by WangZijing on 2020/11/2.
//

#include "rtmp.h"

#include <cstdio>
#include <chrono>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

#include "../utils/log.h"

//'1': Use H.264 Bitstream Filter
#define USE_H264BSF 0

int play_rtmp(const char *stream_url) {

    auto start = std::chrono::steady_clock::now();
    const AVOutputFormat *ofmt;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *ifmt_ctx = nullptr, *ofmt_ctx = nullptr;
    AVPacket pkt;
    const char *in_filename, *out_filename;
    int ret, i;
    int videoindex = -1;
    int frame_index = 0;
    in_filename = stream_url;
    //in_filename  = "rtp://233.233.233.233:6666";
    //out_filename = "receive.ts";
    //out_filename = "receive.mkv";
    out_filename = "/Users/wzjing/Desktop/receive.flv";
    LOGD("play_rtmp :: url = %s\n", stream_url);

    // av_register_all();
    //Network
    avformat_network_init();
    LOGD("start open\n");
    //Input
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, nullptr, nullptr)) < 0) {
        LOGE("Could not open input file.\n");
        goto end;
    }

    LOGD("fetching stream info\n");

    if ((ret = avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) {
        LOGE("Failed to retrieve input stream information\n");
        goto end;
    }

    LOGD("seeking video track\n");

    for (i = 0; i < ifmt_ctx->nb_streams; i++)
        if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGD("found video index %i\n", i);
            videoindex = i;
            break;
        }

    if (videoindex < 0) LOGE("unable to find video track\n");

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    //Output
    avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, out_filename); //RTMP

    if (!ofmt_ctx) {
        LOGE("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        AVStream *in_stream = ifmt_ctx->streams[i];
        const AVCodec *codec_1 = avcodec_find_encoder(in_stream->codecpar->codec_id);
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, codec_1);
        if (!out_stream) {
            LOGE("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        //Copy the settings of AVCodecContext
        const AVCodec *in_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
        AVCodecContext *out_codec_ctx = avcodec_alloc_context3(in_codec);
        ret = avcodec_parameters_to_context(out_codec_ctx, in_stream->codecpar);
        if (ret != 0) {
            LOGE("avcodec_parameters_to_context error\n");
            return -1;
        }
        out_codec_ctx->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        ret = avcodec_parameters_from_context(out_stream->codecpar, out_codec_ctx);
        
        if (ret < 0) {
            printf("Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        // out_stream->codecpar->codec_tag = 0;
        // if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        //     out_stream->codecpar->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    //Dump Format------------------
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    //Open output URL
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output URL '%s'", out_filename);
            goto end;
        }
    }
    //Write file header
    ret = avformat_write_header(ofmt_ctx, nullptr);
    if (ret < 0) {
        LOGE("Error occurred when opening output URL\n");
        goto end;
    }

#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
#endif
    while (true) {
        AVStream *in_stream, *out_stream;
        //Get an AVPacket
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        /* copy packet */
        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //Print to Screen
        if (pkt.stream_index == videoindex) {
            auto end = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            printf("Frame %8d, KeyFrame %d, Time %.3fs\n", frame_index, pkt.flags & AV_PKT_FLAG_KEY,
                   (double) diff / 1000000);
            frame_index++;

#if USE_H264BSF
            av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
        }
        //ret = av_write_frame(ofmt_ctx, &pkt);
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

        if (ret < 0) {
            LOGE("Error muxing packet\n");
            break;
        }

        av_packet_unref(&pkt);

    }

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif

    //Write file trailer
    av_write_trailer(ofmt_ctx);
    end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF) {
        LOGE("Error occurred.\n");
        return -1;
    }
    return 0;
}
