//
// Created by android1 on 2019/5/22.
//

#include "mix_bgm.h"
#include "../utils/log.h"
#include "../utils/error.h"
#include "../filter/audio_filter.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
}

int openAudioFile(const char *file, AVFormatContext *&formatContext, AVCodecContext *&audioContext,
                  AVStream *&audioStream) {
    int ret = 0;
    ret = avformat_open_input(&formatContext, file, nullptr, nullptr);
    if (ret < 0) return error(ret, "input format error");
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) return error(ret, "input format info error");

    for (int j = 0; j < formatContext->nb_streams; ++j) {
        if (formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = formatContext->streams[j];
            const AVCodec *codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
            audioContext = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(audioContext, audioStream->codecpar);
            avcodec_open2(audioContext, codec, nullptr);
        }
    }
    if (!audioStream) {
        LOGE("audio stream not found\n");
        return -1;
    }
    return 0;
}

int openVideoFile(const char *file, AVFormatContext *&formatContext, AVCodecContext *&audioContext,
                  AVCodecContext *&videoContext, AVStream *&audioStream, AVStream *&videoStream) {
    int ret = 0;
    ret = avformat_open_input(&formatContext, file, nullptr, nullptr);
    if (ret < 0) return error(ret, av_err2str(ret));
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) return error(ret, "input format info error");

    for (int j = 0; j < formatContext->nb_streams; ++j) {
        if (formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = formatContext->streams[j];
            const AVCodec *codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
            videoContext = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(videoContext, videoStream->codecpar);
            avcodec_open2(videoContext, codec, nullptr);
        } else if (formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = formatContext->streams[j];
            const AVCodec *codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
            audioContext = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(audioContext, audioStream->codecpar);
            avcodec_open2(audioContext, codec, nullptr);
        }
        if (videoStream && audioStream) break;
    }

    if (!videoStream || !audioContext) {
        LOGD("video or audio stream not found\n");
        return -1;
    }
    return 0;
}

int mix_bgm(const char *output_filename, const char *input_filename, const char *bgm_filename, float bgm_volume) {
    int ret = 0;
    AVFormatContext *outFmtContext = nullptr;
    AVFormatContext *inFmtContext = nullptr;
    AVFormatContext *bgmFmtContext = nullptr;
    AVCodecContext *inAudioContext = nullptr;
    AVCodecContext *inVideoContext = nullptr;
    AVCodecContext *outAudioContext = nullptr;
//    AVCodecContext *outVideoContext = nullptr;
    AVCodecContext *bgmAudioContext = nullptr;

    AVStream *inAudioStream = nullptr;
    AVStream *inVideoStream = nullptr;
    AVStream *outAudioStream = nullptr;
    AVStream *outVideoStream = nullptr;
    AVStream *bgmAudioStream = nullptr;

//    AVCodec *videoCodec = nullptr;
    const AVCodec *audioCodec;

    ret = openVideoFile(input_filename, inFmtContext, inAudioContext, inVideoContext, inAudioStream,
                        inVideoStream);
    if (ret < 0) return ret;
    ret = openAudioFile(bgm_filename, bgmFmtContext, bgmAudioContext, bgmAudioStream);
    if (ret < 0) return ret;

    // configure output
    ret = avformat_alloc_output_context2(&outFmtContext, nullptr, nullptr, output_filename);
    if (ret < 0) return error(ret, "output format error");

    // Copy codec from input video AVStream
//    videoCodec = avcodec_find_encoder(inVideoStream->codecpar->codec_id);
    audioCodec = avcodec_find_encoder(inAudioStream->codecpar->codec_id);

    // create output Video AVStream
    outVideoStream = avformat_new_stream(outFmtContext, nullptr);
    if (!outVideoStream) {
        LOGE("create output video stream error\n");
        return -1;
    }
    outVideoStream->index = outFmtContext->nb_streams - 1;
    ret = avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar);
    if (ret < 0) {
        LOGE("unable to copy video stream code parameter: %s\n", av_err2str(ret));
        return -1;
    }
    outVideoStream->codecpar->codec_tag = 0;

    outAudioStream = avformat_new_stream(outFmtContext, audioCodec);
    if (!outAudioStream) {

        LOGE("create output audio stream error\n");
        return -1;
    }
    outAudioStream->index = outFmtContext->nb_streams - 1;

//    outVideoContext = avcodec_alloc_context3(videoCodec);
//    avcodec_parameters_to_context(outVideoContext, inVideoStream->codecpar);
//    outVideoContext->codec_id = inVideoContext->codec_id;
//    outVideoContext->width = inVideoContext->width;
//    outVideoContext->height = inVideoContext->height;
//    outVideoContext->pix_fmt = inVideoContext->pix_fmt;
//    outVideoContext->bit_rate = inVideoContext->bit_rate;
//    outVideoContext->has_b_frames = inVideoContext->has_b_frames;
//    outVideoContext->gop_size = inVideoContext->gop_size;
//    outVideoContext->qmin = inVideoContext->qmin;
//    outVideoContext->qmax = inVideoContext->qmax;
//    outVideoContext->time_base = (AVRational) {inVideoStream->r_frame_rate.den,
//                                               inVideoStream->r_frame_rate.num};
//    outVideoContext->profile = inVideoContext->profile;
//    outVideoStream->time_base = outVideoContext->time_base;
//    if (outFmtContext->oformat->flags & AVFMT_GLOBALHEADER) {
//        outVideoContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//    }
//    ret = avcodec_open2(outVideoContext, videoCodec, nullptr);
//    if (ret < 0) return error(ret, "Open Video output AVCodecContext");
//    ret = avcodec_parameters_from_context(outVideoStream->codecpar, outVideoContext);
//    if (ret < 0) return error(ret, "Copy Video Context to output stream");
//    av_dict_copy(&outVideoStream->metadata, inVideoStream->metadata, 0);

    // Copy Audio Stream Configure from base Fragment
    outAudioContext = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(outAudioContext, inAudioStream->codecpar);
    outAudioContext->codec_type = inAudioContext->codec_type;
    outAudioContext->codec_id = inAudioContext->codec_id;
    outAudioContext->sample_fmt = inAudioContext->sample_fmt;
    outAudioContext->sample_rate = inAudioContext->sample_rate;
    outAudioContext->bit_rate = inAudioContext->bit_rate;
    outAudioContext->channel_layout = inAudioContext->channel_layout;
    outAudioContext->channels = inAudioContext->channels;
    outAudioContext->time_base = (AVRational) {1, outAudioContext->sample_rate};
    outAudioStream->time_base = outAudioContext->time_base;
    if (outFmtContext->oformat->flags & AVFMT_GLOBALHEADER) {
        outAudioContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    ret = avcodec_open2(outAudioContext, audioCodec, nullptr);
    if (ret < 0) return error(ret, "Open Audio output AVCodecContext");
    ret = avcodec_parameters_from_context(outAudioStream->codecpar, outAudioContext);
    if (ret < 0) return error(ret, "Copy Audio Context to output stream");
    av_dict_copy(&outAudioStream->metadata, inAudioStream->metadata, 0);


    av_dict_copy(&outFmtContext->metadata, inFmtContext->metadata, 0);

    if (!(outFmtContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", output_filename);
        ret = avio_open(&outFmtContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(outFmtContext, nullptr);
    if (ret < 0) return error(ret, "write header error");

    logContext(inVideoContext, "input", 1);
//    logContext(outVideoContext, "out", 1);

    logMetadata(outVideoStream->metadata, "Video-Stream");
//    return 0;

    AVPacket *packet = av_packet_alloc();
    AVPacket *bgmPacket = av_packet_alloc();
    AVPacket *mixPacket = av_packet_alloc();
    AVFrame *inputFrame = av_frame_alloc();
    AVFrame *bgmFrame = av_frame_alloc();

    AudioFilter filter;
    AudioConfig config1{inAudioContext->sample_fmt,
                        inAudioContext->sample_rate,
                        inAudioContext->channel_layout,
                        inAudioContext->time_base};
    AudioConfig config2{bgmAudioContext->sample_fmt,
                        bgmAudioContext->sample_rate,
                        bgmAudioContext->channel_layout,
                        bgmAudioContext->time_base};
    AudioConfig configOut{outAudioContext->sample_fmt,
                          outAudioContext->sample_rate,
                          outAudioContext->channel_layout,
                          outAudioContext->time_base};
    char filter_description[128];
    snprintf(filter_description, sizeof(filter_description), "[in2]volume=volume=%f[out2];[in1][out2]amix[out]",
             bgm_volume);
    filter.create(filter_description, &config1, &config2, &configOut);

    do {
        ret = av_read_frame(inFmtContext, packet);
        if (ret == AVERROR_EOF) {
            LOGW("\tread fragment end of file\n");
            break;
        } else if (ret < 0) {
            LOGE("read fragment error: %s\n", av_err2str(ret));
            break;
        }

        if (packet->flags & AV_PKT_FLAG_DISCARD) continue;
        if (packet->stream_index == inVideoStream->index) {
            packet->stream_index = outVideoStream->index;
            av_packet_rescale_ts(packet, inVideoStream->time_base, outVideoStream->time_base);
            packet->duration = av_rescale_q(packet->duration, inVideoStream->time_base, outVideoStream->time_base);
            packet->pos = -1;
            logPacket(packet, "video");
            ret = av_interleaved_write_frame(outFmtContext, packet);
            if (ret < 0) {
                LOGW("video frame wirte error: %s\n", av_err2str(ret));
            }
            LOGD("\n");
        } else if (packet->stream_index == inAudioStream->index) {

            packet->stream_index = outAudioStream->index;
            av_packet_rescale_ts(packet, inAudioStream->time_base, outAudioStream->time_base);

            int got_bgm = 0;
            // decode bgm packet
            while (true) {
                ret = av_read_frame(bgmFmtContext, bgmPacket);
                if (ret == AVERROR_EOF) {
                    LOGD("read bgm end of file\n");
                    break;
                } else if (ret != 0) {
                    LOGE("read bgm error: %s\n", av_err2str(ret));
                    break;
                }
                if (bgmPacket->stream_index == bgmAudioStream->index) {
                    avcodec_send_packet(bgmAudioContext, bgmPacket);
                    ret = avcodec_receive_frame(bgmAudioContext, bgmFrame);
                    got_bgm = ret == 0;
                    break;
                }
            }


            // decode audio frame
            avcodec_send_packet(inAudioContext, packet);
            ret = avcodec_receive_frame(inAudioContext, inputFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                LOGW("audio frame unavailable\n");
                continue;
            } else if (ret < 0) {
                LOGE("\taudio frame decode error: %s\n", av_err2str(ret));
                return -1;
            }

            AVFrame *mixFrame = av_frame_alloc();
            mixFrame->format = inputFrame->format;
            mixFrame->nb_samples = inputFrame->nb_samples;
            mixFrame->sample_rate = inputFrame->sample_rate;
            mixFrame->channel_layout = inputFrame->channel_layout;
            mixFrame->channels = inputFrame->channels;
            av_frame_get_buffer(mixFrame, 0);
            av_frame_make_writable(mixFrame);

            if (got_bgm) {
                ret = filter.filter(inputFrame, bgmFrame, mixFrame);
                if (ret < 0) {
                    LOGE("\tunable to mix background music: %d\n", ret);
                    return -1;
                }
            } else {
                ret = av_frame_copy(mixFrame, inputFrame);
                if (ret != 0) {
                    LOGW("audio frame copy data error: %s\n", av_err2str(ret));
                }
                ret = av_frame_copy_props(mixFrame, inputFrame);
                if (ret != 0) {
                    LOGW("audio frame copy props error: %s\n", av_err2str(ret));
                }
            }

            avcodec_send_frame(outAudioContext, mixFrame);
            encode:
            ret = avcodec_receive_packet(outAudioContext, mixPacket);
            if (ret == 0) {
                mixPacket->stream_index = outAudioStream->index;
                logPacket(mixPacket, "audio");
                ret = av_interleaved_write_frame(outFmtContext, mixPacket);
                if (ret < 0) {
                    LOGW("audio frame wirte error: %s\n", av_err2str(ret));
                }
                goto encode;
            } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                LOGW("\n");
            } else {
                LOGE("\tunable to encode audio frame: %s\n", av_err2str(ret));
                return -1;
            }
            av_frame_free(&mixFrame);
            LOGD("\n");
        }
    } while (true);

    filter.destroy();

    av_write_trailer(outFmtContext);


    if (!(outFmtContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFmtContext->pb);
    }

    av_frame_free(&inputFrame);
    av_frame_free(&bgmFrame);
    av_packet_free(&packet);
    av_packet_free(&mixPacket);
    av_packet_free(&bgmPacket);

    avformat_free_context(outFmtContext);
    avformat_free_context(inFmtContext);
    avformat_free_context(bgmFmtContext);

    avcodec_free_context(&inAudioContext);
    avcodec_free_context(&inVideoContext);
    avcodec_free_context(&bgmAudioContext);
    avcodec_free_context(&outAudioContext);
//    avcodec_free_context(&outVideoContext);

    return 0;
}
