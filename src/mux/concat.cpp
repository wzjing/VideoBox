//
// Created by android1 on 2019/4/22.
//

#include "concat.h"
#include "../utils/log.h"
#include "../codec/decode.h"
#include "../codec/encode.h"
#include "../filter/mix_filter.h"

int error(const char *message) {
    LOGE("error: file %s line %d\n\t\033[31m%s\033[0m\n", __FILE__, __LINE__, message);
    return -1;
}

void check(int ret, const char *message) {
    if (ret != 0) {
        LOGE("%s: ", message);
        error(av_err2str(ret));
        exit(1);
    }
}

int concat(const char *input_filename, const char *input_filename1, const char *bgm_file, const char *output_filename) {
    int ret = 0;
    // input fragments
    int nb_fragments = 3;
    const char *input_files[nb_fragments];
    input_files[0] = input_filename;
    input_files[1] = input_filename1;
    input_files[2] = bgm_file;

    auto **fragments = (Fragment **) malloc(nb_fragments * sizeof(Framgment *));
    fragments[0] = (Fragment *) malloc(sizeof(Fragment));
    fragments[1] = (Fragment *) malloc(sizeof(Fragment));
    fragments[2] = (Fragment *) malloc(sizeof(Fragment));

    // output video
    AVFormatContext *oFormatContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecContext *videoCodecContext = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVCodec *videoCodec = nullptr;
    AVCodec *audioCodec = nullptr;
    AVPacket *packet;

    for (int i = 0; i < nb_fragments; ++i) {
        ret = avformat_open_input(&fragments[i]->formatContext, input_files[i], nullptr, nullptr);
        check(ret, "input format error");
        ret = avformat_find_stream_info(fragments[i]->formatContext, nullptr);
        check(ret, "input format info error");
        for (int j = 0; j < fragments[i]->formatContext->nb_streams; ++j) {
            if (fragments[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                LOGE("video id: %d\n", j);
                fragments[i]->videoStream = fragments[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(fragments[i]->videoStream->codecpar->codec_id);
                fragments[i]->videoCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(fragments[i]->videoCodecContext, fragments[i]->videoStream->codecpar);
                avcodec_open2(fragments[i]->videoCodecContext, codec, nullptr);
            } else if (fragments[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                LOGE("audio id: %d\n", j);
                fragments[i]->audioStream = fragments[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(fragments[i]->audioStream->codecpar->codec_id);
                fragments[i]->audioCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(fragments[i]->audioCodecContext, fragments[i]->audioStream->codecpar);
                avcodec_open2(fragments[i]->audioCodecContext, codec, nullptr);
            }
            if (fragments[i]->videoStream && fragments[i]->audioStream) break;
        }
        LOGD("Input file: %s: ", input_files[i]);

        fragments[i]->videoStream ? LOGD("Video/") : LOGD("--/");
        fragments[i]->audioStream ? LOGD("Audio\n") : LOGD("--\n");
        av_dump_format(fragments[i]->formatContext, 0, input_files[i], 0);
        LOGD("\n");
    }

    // create output AVFormatContext
    ret = avformat_alloc_output_context2(&oFormatContext, nullptr, nullptr, output_filename);
    check(ret, "output format error");

    // Copy codec from input video AVStream
    videoCodec = avcodec_find_encoder(fragments[0]->videoStream->codecpar->codec_id);
    audioCodec = avcodec_find_encoder(fragments[0]->audioStream->codecpar->codec_id);

    // create output Video AVStream
    videoStream = avformat_new_stream(oFormatContext, videoCodec);
    if (!videoStream) return error("create output video stream error");
    videoStream->id = 0;

    audioStream = avformat_new_stream(oFormatContext, audioCodec);
    if (!audioStream) return error("create output audio stream error");
    audioStream->id = 1;

    // Copy Video Stream Configure
    videoCodecContext = avcodec_alloc_context3(videoCodec);
    ret = avcodec_parameters_to_context(videoCodecContext, fragments[0]->videoStream->codecpar);
    check(ret, "Copy Video Context from input stream");
    videoCodecContext->time_base = AVRational{1, fragments[0]->videoStream->avg_frame_rate.num};
    videoCodecContext->pix_fmt = (AVPixelFormat) fragments[0]->videoStream->codecpar->format;
    videoStream->time_base = videoCodecContext->time_base;
    ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
    check(ret, "Open Video output AVCodecContext");
    ret = avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);
    check(ret, "Copy Video Context to output stream");

    // Copy Audio Stream Configure
    audioCodecContext = avcodec_alloc_context3(audioCodec);
    ret = avcodec_parameters_to_context(audioCodecContext, fragments[0]->audioStream->codecpar);
    check(ret, "Copy Audio Context from input stream");
    audioCodecContext->time_base = AVRational{1, fragments[0]->audioStream->codecpar->sample_rate};
    audioCodecContext->sample_fmt = (AVSampleFormat) fragments[0]->audioStream->codecpar->format;
    audioStream->time_base = audioCodecContext->time_base;
    ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
    check(ret, "Open Audio output AVCodecContext");
    ret = avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext);
    check(ret, "Copy Audio Context to output stream");

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
         videoCodecContext->framerate.num,
         videoCodecContext->framerate.den,
         avcodec_get_name(videoCodecContext->codec_id),
         videoCodecContext->bit_rate,
         fragments[1]->videoStream->time_base.num,
         fragments[1]->videoStream->time_base.den,
         videoStream->time_base.num,
         videoStream->time_base.den);

    packet = av_packet_alloc();
    if (!packet) return error("packet error");

    uint64_t last_video_pts = 0;
    uint64_t last_video_dts = 0;
    uint64_t last_audio_pts = 0;
    uint64_t last_audio_dts = 0;
    AVFrame *audioFrame = av_frame_alloc();
    AVFrame *bgmFrame = av_frame_alloc();
    AVPacket *audioPacket = av_packet_alloc();
    AVPacket *bgmPacket = av_packet_alloc();

    Fragment *bgmFragment = fragments[nb_fragments - 1];
    auto *bgmCodecContext = bgmFragment->audioCodecContext;
    AudioMixFilter filter(bgmCodecContext->channel_layout, bgmCodecContext->sample_fmt, bgmCodecContext->sample_rate,
                          0.6, 1.6);

    filter.init();

    for (int i = 0; i < nb_fragments - 1; ++i) {
        AVFormatContext *inFormatContext = fragments[i]->formatContext;
        AVStream *inVideoStream = fragments[i]->videoStream;
        AVStream *inAudioStream = fragments[i]->audioStream;
//        AVCodecContext *inVideoCodecContext = fragments[i]->videoCodecContext;
        AVCodecContext *inAudioCodecContext = fragments[i]->audioCodecContext;
        uint64_t next_video_pts = 0;
        uint64_t next_video_dts = 0;
        uint64_t next_audio_pts = 0;
        uint64_t next_audio_dts = 0;
        do {
            ret = av_read_frame(inFormatContext, packet);
            if (ret != 0) {
                LOGE("RET: %s\n", av_err2str(ret));
                break;
            }
            if (packet->stream_index == inVideoStream->index) {
                packet->stream_index = videoStream->index;
                av_packet_rescale_ts(packet, inVideoStream->time_base, videoStream->time_base);
                packet->pts += last_video_pts;
                packet->dts += last_video_dts;
                next_video_pts = packet->pts + packet->duration;
                next_video_dts = packet->dts + packet->duration;
//                LOGD("packet-> stream: %d\tPTS: %ld\tDTS: %ld\n", packet->stream_index, packet->dts, packet->pts);
                av_interleaved_write_frame(oFormatContext, packet);
            } else if (packet->stream_index == inAudioStream->index) {
                packet->stream_index = audioStream->index;
                av_packet_rescale_ts(packet, inAudioStream->time_base, audioStream->time_base);
                packet->pts += last_audio_pts;
                packet->dts += last_audio_dts;
                next_audio_pts = packet->pts + packet->duration;
                next_audio_dts = packet->dts + packet->duration;
                LOGD("packet-> stream: %d\tsize:%d\tPTS: %ld\tDTS: %ld\n", packet->stream_index, packet->size,
                     packet->dts, packet->pts);

                // decode bgm frame
                while (true) {
                    ret = av_read_frame(inFormatContext, bgmPacket);
                    if (ret != 0) {
                        LOGE("RET: %s\n", av_err2str(ret));
                        break;
                    }
                    if (bgmPacket->stream_index == bgmFragment->audioStream->index) break;
                }
                decode_packet(bgmFragment->audioCodecContext, bgmFrame, bgmPacket, [](AVFrame *frame) -> void {
                    LOGD("\t got bgm frame\n");
                });

                // decode video frame
                decode_packet(inAudioCodecContext, audioFrame, packet,
                              [&audioCodecContext, &audioPacket, &filter, &bgmFrame](AVFrame *frame) -> void {
//                                  LOGD("Audio Frame-> pts: %ld\tdts: %ld\tfmt: %s\tnb_samples: %d\tchannels: %d\n",
//                                       frame->pts, frame->pkt_dts,
//                                       av_get_sample_fmt_name((AVSampleFormat) frame->format),
//                                       frame->nb_samples, frame->channels);
                                  filter.filter(frame, bgmFrame);
                                  encode_packet(audioCodecContext, frame, audioPacket, [](AVPacket *enc_pkt) -> int {
                                      LOGD("\tre-encode-> size:%d\tPTS: %ld\tDTS: %ld\n", enc_pkt->size, enc_pkt->pts,
                                           enc_pkt->dts);
                                      return 0;
                                  });
                              });
                av_interleaved_write_frame(oFormatContext, packet);
            }
        } while (true);
        last_video_pts = next_video_pts;
        last_video_dts = next_video_dts;
        last_audio_pts = next_audio_pts;
        last_audio_dts = next_audio_dts;
    }

    filter.destroy();

    av_write_trailer(oFormatContext);

    av_frame_free(&audioFrame);

    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&oFormatContext->pb);
    }

    av_packet_free(&packet);

    for (int i = 0; i < nb_fragments; ++i) {
        avformat_free_context(fragments[i]->formatContext);
        if (fragments[i]->videoCodecContext)
            avcodec_free_context(&fragments[i]->videoCodecContext);
        if (fragments[i]->audioCodecContext)
            avcodec_free_context(&fragments[i]->audioCodecContext);
    }

    avformat_free_context(oFormatContext);
    return 0;
}
