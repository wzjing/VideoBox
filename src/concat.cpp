//
// Created by android1 on 2019/4/22.
//

#include "concat.h"
#include "codec/decode.h"
#include "codec/encode.h"
#include "filter/mix_filter.h"

int concat_with_bgm(const char *output_filename, const char *bgm_file, char **video_files, int nb_videos) {
    int ret = 0;
    // input fragments
    int nb_fragments = 3;
    const char *input_files[nb_videos + 1];

    for (int i = 0; i < nb_videos; ++i) {
        input_files[i] = video_files[i];
        LOGD("input file: %s\n", input_files[i]);
    }
    input_files[nb_videos] = bgm_file;

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
        if (ret < 0) return error("input format error");
        ret = avformat_find_stream_info(fragments[i]->formatContext, nullptr);
        if (ret < 0) return error("input format info error");
        for (int j = 0; j < fragments[i]->formatContext->nb_streams; ++j) {
            if (fragments[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                fragments[i]->videoStream = fragments[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(fragments[i]->videoStream->codecpar->codec_id);
                fragments[i]->videoCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(fragments[i]->videoCodecContext, fragments[i]->videoStream->codecpar);
                avcodec_open2(fragments[i]->videoCodecContext, codec, nullptr);
            } else if (fragments[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                fragments[i]->audioStream = fragments[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(fragments[i]->audioStream->codecpar->codec_id);
                fragments[i]->audioCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(fragments[i]->audioCodecContext, fragments[i]->audioStream->codecpar);
                avcodec_open2(fragments[i]->audioCodecContext, codec, nullptr);
            }
            if (fragments[i]->videoStream && fragments[i]->audioStream) break;
        }
        LOGD("\n");
        fragments[i]->videoStream ? LOGD("Video/") : LOGD("--/");
        fragments[i]->audioStream ? LOGD("Audio") : LOGD("--");
        LOGD(": %s\n", input_files[i]);
//        av_dump_format(fragments[i]->formatContext, 0, input_files[i], 0);
        LOGD("\n");
    }

    // create output AVFormatContext
    ret = avformat_alloc_output_context2(&oFormatContext, nullptr, nullptr, output_filename);
    if (ret < 0) return error("output format error");

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

    Fragment *baseFragment = fragments[0]; // the result code is based on this video

    // Copy Video Stream Configure from base Fragment
    videoCodecContext = avcodec_alloc_context3(videoCodec);
    ret = avcodec_parameters_to_context(videoCodecContext, baseFragment->videoStream->codecpar);
    if (ret < 0) return error("Copy Video Context from input stream");
    videoCodecContext->time_base = AVRational{1, baseFragment->videoStream->avg_frame_rate.num};
    videoCodecContext->pix_fmt = (AVPixelFormat) baseFragment->videoStream->codecpar->format;
    videoStream->time_base = videoCodecContext->time_base;
    ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
    if (ret < 0) return error("Open Video output AVCodecContext");
    ret = avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);
    if (ret < 0) return error("Copy Video Context to output stream");

    // Copy Audio Stream Configure from base Fragment
    audioCodecContext = avcodec_alloc_context3(audioCodec);
    ret = avcodec_parameters_to_context(audioCodecContext, baseFragment->audioStream->codecpar);
    if (ret < 0) return error("Copy Audio Context from input stream");
    audioCodecContext->time_base = AVRational{1, baseFragment->audioStream->codecpar->sample_rate};
    audioCodecContext->sample_fmt = (AVSampleFormat) baseFragment->audioStream->codecpar->format;
    audioStream->time_base = audioCodecContext->time_base;
    ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
    if (ret < 0) return error("Open Audio output AVCodecContext");
    ret = avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext);
    if (ret < 0) return error("Copy Audio Context to output stream");

    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", output_filename);
        ret = avio_open(&oFormatContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(oFormatContext, nullptr);
    if (ret < 0) return error("write header error");

    LOGD("contexts\n");
    logContext(fragments[0]->videoCodecContext, "Frag0", 1);
    logContext(fragments[1]->videoCodecContext, "Frag1", 1);
    logContext(videoCodecContext, "Result", 1);
    logContext(fragments[0]->audioCodecContext, "Frag0", 0);
    logContext(fragments[1]->audioCodecContext, "Frag1", 0);
    logContext(audioCodecContext, "Result", 0);
    LOGD("\n");

    LOGD("streams:\n");
    logStream(fragments[0]->videoStream, "Frag0", 1);
    logStream(fragments[1]->videoStream, "Frag1", 1);
    logStream(videoStream, "Result", 1);
    logStream(fragments[0]->audioStream, "Frag0", 0);
    logStream(fragments[1]->audioStream, "Frag1", 0);
    logStream(audioStream, "Result", 0);
    LOGD("\n");

//    av_dump_format(oFormatContext, 0, output_filename, 1);

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
    AVFrame *mixFrame = av_frame_alloc();
    AVPacket *audioPacket = av_packet_alloc();
    AVPacket *bgmPacket = av_packet_alloc();
    int gotAudioFrame = 0;

    Fragment *bgmFragment = fragments[nb_fragments - 1];
    auto *bgmCodecContext = bgmFragment->audioCodecContext;
    AudioMixFilter filter(audioCodecContext->channel_layout,
                          bgmCodecContext->channel_layout,
                          audioCodecContext->sample_fmt,
                          bgmCodecContext->sample_fmt,
                          audioCodecContext->sample_rate,
                          bgmCodecContext->sample_rate,
                          0.4, 1.2);
    LOGD("filter: rate:%d\n", bgmCodecContext->sample_rate);

    filter.init();

    for (int i = 0; i < nb_fragments - 1; ++i) {
        AVFormatContext *inFormatContext = fragments[i]->formatContext;
        AVStream *inVideoStream = fragments[i]->videoStream;
        AVStream *inAudioStream = fragments[i]->audioStream;
        AVCodecContext *inAudioCodecContext = fragments[i]->audioCodecContext;
        uint64_t next_video_pts = 0;
        uint64_t next_video_dts = 0;
        uint64_t next_audio_pts = 0;
        uint64_t next_audio_dts = 0;
        do {
            ret = av_read_frame(inFormatContext, packet);
            if (ret == AVERROR_EOF) {
                LOGW("\tread fragment end of file\n");
                break;
            } else if (ret < 0) {
                LOGE("read fragment error: %s\n", av_err2str(ret));
                break;
            }
            if (packet->dts < 0) continue;
            if (packet->stream_index == inVideoStream->index) {
                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;

                packet->stream_index = videoStream->index;

                av_packet_rescale_ts(packet, inVideoStream->time_base, videoStream->time_base);
                packet->pts += last_video_pts;
                packet->dts += last_video_dts;
                next_video_pts = packet->pts + packet->duration;
                next_video_dts = packet->dts + packet->duration;
//                LOGD("\033[32mVideo\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\n", old_pts, old_dts, packet->pts,
//                     packet->dts);
                av_interleaved_write_frame(oFormatContext, packet);
            } else if (packet->stream_index == inAudioStream->index) {
                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;

                packet->stream_index = audioStream->index;
                av_packet_rescale_ts(packet, inAudioStream->time_base, audioStream->time_base);
                packet->pts += last_audio_pts;
                packet->dts += last_audio_dts;
                next_audio_pts = packet->pts + packet->duration;
                next_audio_dts = packet->dts + packet->duration;
                logPacket(packet, "origin");

                // decode bgm packet
                while (true) {
                    ret = av_read_frame(bgmFragment->formatContext, bgmPacket);
                    if (ret == AVERROR_EOF) {
                        LOGD("read bgm end of file\n");
                        break;
                    } else if (ret != 0) {
                        LOGE("read bgm error: %s\n", av_err2str(ret));
                        break;
                    }
                    if (bgmPacket->stream_index == bgmFragment->audioStream->index) break;
                }

                // decode bgm frame
                avcodec_send_packet(bgmCodecContext, bgmPacket);
                ret = avcodec_receive_frame(bgmCodecContext, bgmFrame);
//                logFrame(bgmFrame, "bgm", 0);
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    LOGE("\tunable to decode background music frame: %s\n", av_err2str(ret));
                    goto error;
                }


                // decode audio frame
                avcodec_send_packet(inAudioCodecContext, packet);
                ret = avcodec_receive_frame(inAudioCodecContext, audioFrame);
                gotAudioFrame = ret == 0;
//                logFrame(audioFrame, "origin", 0);
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    LOGE("\tunable to decode input video music frame: %s\n", av_err2str(ret));
                    goto error;
                }

                AVFrame *destAudioFrame;
                // filter frame
                if (!gotAudioFrame) {
                    audioFrame->pts = packet->pts;
                    audioFrame->nb_samples = bgmFrame->nb_samples;
                    audioFrame->channel_layout = audioCodecContext->channel_layout;
                    audioFrame->format = audioCodecContext->sample_fmt;
                    audioFrame->sample_rate = audioCodecContext->sample_rate;
                    ret = av_frame_get_buffer(audioFrame, 0);
                    if (ret != 0) {
                        LOGW("\tunable to fill audio frame: %s\n", av_err2str(ret));
                        continue;
                    }
                    int sample_size = av_get_bytes_per_sample((AVSampleFormat) audioFrame->format);
                    for (i = 0; i < audioFrame->channels; i++) {
                        LOGW("\tclean %d/%d\n", i, audioFrame->linesize[i]);
                        memset(audioFrame->data[i], '0', audioFrame->nb_samples * sample_size);
                    }

                }

                ret = filter.filter(audioFrame, bgmFrame, mixFrame);
                if (ret == 0) {
                    LOGD("\tgot mix frame\n");
                    destAudioFrame = mixFrame;
                } else if (ret == AVERROR(EAGAIN)) {
                    logFrame(audioFrame, "Error Audio", 0);
                    destAudioFrame = audioFrame;
                } else if (ret == AVERROR_EOF) {
                    LOGW("\tbgm eof\n");
                    destAudioFrame = audioFrame;
                } else {
                    LOGE("\tunable to mix background music: %d\n", ret);
                    goto error;
                }


                // re-encode frame
                logFrame(destAudioFrame, "dest", 0);
                avcodec_send_frame(audioCodecContext, destAudioFrame);
                encode:
                ret = avcodec_receive_packet(audioCodecContext, audioPacket);
                if (ret == 0) {
                    av_packet_rescale_ts(audioPacket, audioCodecContext->time_base, audioStream->time_base);
                    audioPacket->stream_index = audioStream->index;
                    logPacket(audioPacket, "encoded");
                    LOGD("\033[32mAudio\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\n", old_pts, old_dts,
                         audioPacket->pts,
                         audioPacket->dts);
                    av_interleaved_write_frame(oFormatContext, audioPacket);
                    goto encode;
                } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    LOGW("\tencode warning: %s\n", av_err2str(ret));
                } else {
                    LOGE("\tunable to encode audio frame: %s\n", av_err2str(ret));
                    goto error;
                }
                av_frame_unref(destAudioFrame);
            }
        } while (true);
        last_video_pts = next_video_pts;
        last_video_dts = next_video_dts;
        last_audio_pts = next_audio_pts;
        last_audio_dts = next_audio_dts;

        LOGD("--------------------------------------------------------------------------------\n");
    }


    av_write_trailer(oFormatContext);


    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&oFormatContext->pb);
    }

    filter.destroy();
    av_frame_free(&audioFrame);
    av_frame_free(&bgmFrame);
    av_frame_free(&mixFrame);
    av_packet_free(&packet);
    av_packet_free(&audioPacket);
    av_packet_free(&bgmPacket);

    for (int i = 0; i < nb_fragments; ++i) {
        avformat_free_context(fragments[i]->formatContext);
        if (fragments[i]->videoCodecContext)
            avcodec_free_context(&fragments[i]->videoCodecContext);
        if (fragments[i]->audioCodecContext)
            avcodec_free_context(&fragments[i]->audioCodecContext);
    }

    avformat_free_context(oFormatContext);
    return 0;
    error:
    filter.destroy();
    av_frame_free(&audioFrame);
    av_frame_free(&bgmFrame);
    av_frame_free(&mixFrame);
    av_packet_free(&packet);
    av_packet_free(&audioPacket);
    av_packet_free(&bgmPacket);
    for (int i = 0; i < nb_fragments; ++i) {
        avformat_free_context(fragments[i]->formatContext);
        if (fragments[i]->videoCodecContext)
            avcodec_free_context(&fragments[i]->videoCodecContext);
        if (fragments[i]->audioCodecContext)
            avcodec_free_context(&fragments[i]->audioCodecContext);
    }
    avformat_free_context(oFormatContext);
    return -1;
}

int concat(const char *output_filename, char **video_files, int nb_videos) {
    int ret = 0;
    // input fragments

    auto **fragments = (Fragment **) malloc(nb_videos * sizeof(Framgment *));

    for (int i = 0; i < nb_videos; i++) {
        fragments[i] = (Fragment *) malloc(sizeof(Fragment));
    }

    // output video
    AVFormatContext *oFormatContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecContext *videoCodecContext = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVCodec *videoCodec = nullptr;
    AVCodec *audioCodec = nullptr;
    AVPacket *packet;

    for (int i = 0; i < nb_videos; ++i) {
        ret = avformat_open_input(&fragments[i]->formatContext, video_files[i], nullptr, nullptr);
        if (ret < 0) return error("input format error");
        ret = avformat_find_stream_info(fragments[i]->formatContext, nullptr);
        if (ret < 0) return error("input format info error");
        for (int j = 0; j < fragments[i]->formatContext->nb_streams; ++j) {
            if (fragments[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                fragments[i]->videoStream = fragments[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(fragments[i]->videoStream->codecpar->codec_id);
                fragments[i]->videoCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(fragments[i]->videoCodecContext, fragments[i]->videoStream->codecpar);
                avcodec_open2(fragments[i]->videoCodecContext, codec, nullptr);
            } else if (fragments[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                fragments[i]->audioStream = fragments[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(fragments[i]->audioStream->codecpar->codec_id);
                fragments[i]->audioCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(fragments[i]->audioCodecContext, fragments[i]->audioStream->codecpar);
                avcodec_open2(fragments[i]->audioCodecContext, codec, nullptr);
            }
            if (fragments[i]->videoStream && fragments[i]->audioStream) break;
        }
        LOGD("\n");
        fragments[i]->videoStream ? LOGD("Video/") : LOGD("--/");
        fragments[i]->audioStream ? LOGD("Audio") : LOGD("--");
        LOGD(": %s\n", video_files[i]);
//        av_dump_format(fragments[i]->formatContext, 0, input_files[i], 0);
        LOGD("\n");
    }

    // create output AVFormatContext
    ret = avformat_alloc_output_context2(&oFormatContext, nullptr, nullptr, output_filename);
    if (ret < 0) return error("output format error");

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

    Fragment *baseFragment = fragments[0]; // the result code is based on this video

    // Copy Video Stream Configure from base Fragment
    videoCodecContext = avcodec_alloc_context3(videoCodec);
    ret = avcodec_parameters_to_context(videoCodecContext, baseFragment->videoStream->codecpar);
    if (ret < 0) return error("Copy Video Context from input stream");
    videoCodecContext->time_base = AVRational{1, baseFragment->videoStream->avg_frame_rate.num};
    videoCodecContext->pix_fmt = (AVPixelFormat) baseFragment->videoStream->codecpar->format;
    videoStream->time_base = videoCodecContext->time_base;
    ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
    if (ret < 0) return error("Open Video output AVCodecContext");
    ret = avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);
    if (ret < 0) return error("Copy Video Context to output stream");

    // Copy Audio Stream Configure from base Fragment
    audioCodecContext = avcodec_alloc_context3(audioCodec);
    ret = avcodec_parameters_to_context(audioCodecContext, baseFragment->audioStream->codecpar);
    if (ret < 0) return error("Copy Audio Context from input stream");
    audioCodecContext->time_base = AVRational{1, baseFragment->audioStream->codecpar->sample_rate};
    audioCodecContext->sample_fmt = (AVSampleFormat) baseFragment->audioStream->codecpar->format;
    audioStream->time_base = audioCodecContext->time_base;
    ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
    if (ret < 0) return error("Open Audio output AVCodecContext");
    ret = avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext);
    if (ret < 0) return error("Copy Audio Context to output stream");

    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", output_filename);
        ret = avio_open(&oFormatContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(oFormatContext, nullptr);
    if (ret < 0) return error("write header error");

    LOGD("contexts\n");
    logContext(fragments[0]->videoCodecContext, "Frag0", 1);
    logContext(fragments[1]->videoCodecContext, "Frag1", 1);
    logContext(videoCodecContext, "Result", 1);
    logContext(fragments[0]->audioCodecContext, "Frag0", 0);
    logContext(fragments[1]->audioCodecContext, "Frag1", 0);
    logContext(audioCodecContext, "Result", 0);
    LOGD("\n");

    LOGD("streams:\n");
    logStream(fragments[0]->videoStream, "Frag0", 1);
    logStream(fragments[1]->videoStream, "Frag1", 1);
    logStream(videoStream, "Result", 1);
    logStream(fragments[0]->audioStream, "Frag0", 0);
    logStream(fragments[1]->audioStream, "Frag1", 0);
    logStream(audioStream, "Result", 0);
    LOGD("\n");

//    av_dump_format(oFormatContext, 0, output_filename, 1);

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
    AVPacket *audioPacket = av_packet_alloc();

    for (int i = 0; i < nb_videos; ++i) {
        AVFormatContext *inFormatContext = fragments[i]->formatContext;
        AVStream *inVideoStream = fragments[i]->videoStream;
        AVStream *inAudioStream = fragments[i]->audioStream;
        uint64_t next_video_pts = 0;
        uint64_t next_video_dts = 0;
        uint64_t next_audio_pts = 0;
        uint64_t next_audio_dts = 0;
        if (inVideoStream->start_time != AV_NOPTS_VALUE) {
            int64_t offset = av_rescale_q(inVideoStream->start_time, videoStream->time_base, inVideoStream->time_base);
            LOGD("offset video: %ld -> %ld\n", inVideoStream->start_time, offset);
            last_video_pts -= offset;
            last_video_dts -= offset;
        }
        if (inVideoStream->start_time != AV_NOPTS_VALUE) {
            int64_t offset = av_rescale_q(inAudioStream->start_time, audioStream->time_base, inAudioStream->time_base);
            LOGD("offset audio: %ld -> %ld\n", inAudioStream->start_time, offset);
            last_audio_pts -= offset;
            last_audio_dts -= offset;
        }
        LOGW("Audio-> PTS:%ld DTS%ld Video-> PTS:%ld DTS:%ld\n", last_audio_pts, last_audio_dts, last_video_pts, last_video_dts);
        do {
            ret = av_read_frame(inFormatContext, packet);
            if (ret == AVERROR_EOF) {
                LOGW("\tread fragment end of file\n");
                break;
            } else if (ret < 0) {
                LOGE("read fragment error: %s\n", av_err2str(ret));
                break;
            }
//            if (packet->dts < 0) continue;
            if (packet->stream_index == inVideoStream->index) {
                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;
                packet->stream_index = videoStream->index;

                av_packet_rescale_ts(packet, inVideoStream->time_base, videoStream->time_base);
                packet->pts += last_video_pts;
                packet->dts += last_video_dts;
                next_video_pts = packet->pts + packet->duration;
                next_video_dts = packet->dts + packet->duration;
                LOGD("\033[32mVIDEO\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\tKEY:%s\n",
                     old_pts, old_dts,
                     packet->pts,
                     packet->dts,
                     packet->flags & AV_PKT_FLAG_KEY ? "Key" : "--");
                av_interleaved_write_frame(oFormatContext, packet);
            } else if (packet->stream_index == inAudioStream->index) {
                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;
                packet->stream_index = audioStream->index;

                av_packet_rescale_ts(packet, inAudioStream->time_base, audioStream->time_base);
                packet->pts += last_audio_pts;
                packet->dts += last_audio_dts;
                next_audio_pts = packet->pts + packet->duration;
                next_audio_dts = packet->dts + packet->duration;
//                LOGD("\033[32mAUDIO\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\n", old_pts, old_dts, packet->pts,
//                     packet->dts);
                av_interleaved_write_frame(oFormatContext, packet);
            }
        } while (true);
        last_video_pts = next_video_pts;
        last_video_dts = next_video_dts;
        last_audio_pts = next_audio_pts;
        last_audio_dts = next_audio_dts;

        LOGD("--------------------------------------------------------------------------------\n");
    }


    av_write_trailer(oFormatContext);


    if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&oFormatContext->pb);
    }

    av_frame_free(&audioFrame);
    av_packet_free(&packet);
    av_packet_free(&audioPacket);

    for (int i = 0; i < nb_videos; ++i) {
        avformat_free_context(fragments[i]->formatContext);
        if (fragments[i]->videoCodecContext)
            avcodec_free_context(&fragments[i]->videoCodecContext);
        if (fragments[i]->audioCodecContext)
            avcodec_free_context(&fragments[i]->audioCodecContext);
    }

    avformat_free_context(oFormatContext);
    return 0;
}
