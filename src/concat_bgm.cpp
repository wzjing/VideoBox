#include "concat_bgm.h"
#include "filter/mix_filter.h"

int concat(const char *output_filename, char *bgm_file, char **video_files, int nb_videos) {
    int ret = 0;
    // input fragments
    auto **fragments = (Fragment **) malloc((nb_videos + 1) * sizeof(Framgment *));
    auto files = (char **) malloc((nb_videos + 1) * sizeof(char *));

    for (int i = 0; i < nb_videos + 1; i++) {
        fragments[i] = (Fragment *) malloc(sizeof(Fragment));
        files[i] = video_files[i];
    }
    files[nb_videos] = bgm_file;

    // output video
    AVFormatContext *oFormatContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecContext *videoCodecContext = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVCodec *videoCodec = nullptr;
    AVCodec *audioCodec = nullptr;

    for (int i = 0; i < nb_videos + 1; ++i) {
        ret = avformat_open_input(&fragments[i]->formatContext, files[i], nullptr, nullptr);
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
        LOGD(": %s\n", files[i]);
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

    Fragment *baseFragment = fragments[1]; // the result code is based on this video

    // Copy Video Stream Configure from base Fragment
    videoCodecContext = avcodec_alloc_context3(videoCodec);
//    ret = avcodec_parameters_to_context(videoCodecContext, baseFragment->videoStream->codecpar);
//    if (ret < 0) return error("Copy Video Context from input stream");
    videoCodecContext->codec_id = baseFragment->videoCodecContext->codec_id;
    videoCodecContext->width = baseFragment->videoCodecContext->width;
    videoCodecContext->height = baseFragment->videoCodecContext->height;
    videoCodecContext->pix_fmt = baseFragment->videoCodecContext->pix_fmt;
    videoCodecContext->bit_rate = baseFragment->videoCodecContext->bit_rate;
    videoCodecContext->has_b_frames = baseFragment->videoCodecContext->has_b_frames;
    videoCodecContext->gop_size = baseFragment->videoCodecContext->gop_size;
    videoCodecContext->qmin = baseFragment->videoCodecContext->qmin;
    videoCodecContext->qmax = baseFragment->videoCodecContext->qmax;
    videoCodecContext->time_base = (AVRational) {baseFragment->videoStream->r_frame_rate.den,
                                                 baseFragment->videoStream->r_frame_rate.num};
    videoCodecContext->profile = baseFragment->videoCodecContext->profile;
    videoStream->time_base = videoCodecContext->time_base;
    ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
    if (ret < 0) return error("Open Video output AVCodecContext");
    ret = avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);
    if (ret < 0) return error("Copy Video Context to output stream");

    // Copy Audio Stream Configure from base Fragment
    audioCodecContext = avcodec_alloc_context3(audioCodec);
//    ret = avcodec_parameters_to_context(audioCodecContext, baseFragment->audioStream->codecpar);
//    if (ret < 0) return error("Copy Audio Context from input stream");
    audioCodecContext->codec_type = baseFragment->audioCodecContext->codec_type;
    audioCodecContext->codec_id = baseFragment->audioCodecContext->codec_id;
    audioCodecContext->sample_fmt = baseFragment->audioCodecContext->sample_fmt;
    audioCodecContext->sample_rate = baseFragment->audioCodecContext->sample_rate;
    audioCodecContext->bit_rate = baseFragment->audioCodecContext->bit_rate;
    audioCodecContext->channel_layout = baseFragment->audioCodecContext->channel_layout;
    audioCodecContext->channels = baseFragment->audioCodecContext->channels;
    audioCodecContext->time_base = (AVRational) {1, audioCodecContext->sample_rate};
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


    ret = avformat_write_header(oFormatContext, nullptr);
    if (ret < 0) return error("write header error");


    av_dump_format(oFormatContext, 0, output_filename, 1);

    Fragment *bgmFragment = fragments[nb_videos];
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

    AVPacket *packet = av_packet_alloc();
    AVPacket *mixPacket = av_packet_alloc();
    AVPacket *bgmPacket = av_packet_alloc();
    AVFrame *audioFrame = av_frame_alloc();
    AVFrame *bgmFrame = av_frame_alloc();

    if (!packet) return error("packet error");

    uint64_t last_video_pts = 0;
    uint64_t last_video_dts = 0;
    uint64_t last_audio_pts = 0;
    uint64_t last_audio_dts = 0;


    for (int i = 0; i < nb_videos; ++i) {
        AVFormatContext *inFormatContext = fragments[i]->formatContext;
        AVCodecContext *inAudioCodecContext = fragments[i]->audioCodecContext;

        AVStream *inVideoStream = fragments[i]->videoStream;
        AVStream *inAudioStream = fragments[i]->audioStream;

        uint64_t first_video_pts = 0;
        uint64_t first_video_dts = 0;
        uint64_t first_audio_pts = 0;
        uint64_t first_audio_dts = 0;
        int video_ts_set = 0;
        int audio_ts_set = 0;

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
                if (packet->flags & AV_PKT_FLAG_DISCARD) {
                    LOGW("\nPacket is discard\n");
                    continue;
                }

                if (!video_ts_set) {
                    first_video_pts = packet->pts;
                    first_video_dts = packet->dts;
                    video_ts_set = 1;
                }

                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;
                packet->stream_index = videoStream->index;

                packet->pts -= first_video_pts;
                packet->dts -= first_video_dts;

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
                logPacket(packet, "origin");
                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;

                if (!audio_ts_set) {
                    first_audio_pts = packet->pts;
                    first_audio_dts = packet->dts;
                    audio_ts_set = 1;
                }
                packet->pts -= first_audio_pts;
                packet->dts -= first_audio_dts;
                av_packet_rescale_ts(packet, inAudioStream->time_base, audioStream->time_base);
                packet->pts += last_audio_pts;
                packet->dts += last_audio_dts;
                next_audio_pts = packet->pts + packet->duration;
                next_audio_dts = packet->dts + packet->duration;
                packet->stream_index = audioStream->index;

                int got_bgm = 0;
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
                    if (bgmPacket->stream_index == bgmFragment->audioStream->index) {
                        avcodec_send_packet(bgmCodecContext, bgmPacket);
                        ret = avcodec_receive_frame(bgmCodecContext, bgmFrame);
                        got_bgm = ret == 0;
                        break;
                    }
                }


                // decode audio frame
                avcodec_send_packet(inAudioCodecContext, packet);
                ret = avcodec_receive_frame(inAudioCodecContext, audioFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    LOGW("audio frame unavailable\n");
                    continue;
                } else if (ret < 0) {
                    LOGE("\taudio frame decode error: %s\n", av_err2str(ret));
                    return -1;
                }
                logFrame(audioFrame, "origin", 0);

                AVFrame *mixFrame = av_frame_alloc();
                mixFrame->format = audioFrame->format;
                mixFrame->nb_samples = audioFrame->nb_samples;
                mixFrame->sample_rate = audioFrame->sample_rate;
                mixFrame->channel_layout = audioFrame->channel_layout;
                mixFrame->channels = audioFrame->channels;
                av_frame_get_buffer(mixFrame, 0);
                av_frame_make_writable(mixFrame);
                LOGD("writable: %s\n", av_frame_is_writable(mixFrame) ? "TRUE" : "FALSE");

                if (got_bgm) {
                    ret = filter.filter(audioFrame, bgmFrame, mixFrame);
                    if (ret < 0) {
                        LOGE("\tunable to mix background music: %d\n", ret);
                        return -1;
                    }
                } else {
                    ret = av_frame_copy(mixFrame, audioFrame);
                    if (ret != 0) {
                        LOGW("copy error: %s\n", av_err2str(ret));
                    }
                }

                avcodec_send_frame(audioCodecContext, mixFrame);
                encode:
                ret = avcodec_receive_packet(audioCodecContext, mixPacket);
                if (ret == 0) {
                    LOGD("timebase: {%d, %d}->{%d, %d}\n",
                         audioCodecContext->time_base.num,
                         audioCodecContext->time_base.den,
                         audioStream->time_base.num,
                         audioStream->time_base.den);
//                    av_packet_rescale_ts(mixPacket, audioCodecContext->time_base, audioStream->time_base);
                    mixPacket->stream_index = audioStream->index;
                    logPacket(mixPacket, "encoded");
                    LOGD("\033[32mAudio\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\n", old_pts, old_dts,
                         mixPacket->pts,
                         mixPacket->dts);
                    av_interleaved_write_frame(oFormatContext, mixPacket);
                    goto encode;
                } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    LOGW("\tencode warning: %s\n", av_err2str(ret));
                } else {
                    LOGE("\tunable to encode audio frame: %s\n", av_err2str(ret));
                    return -1;
                }
                LOG("\n");
                av_frame_free(&mixFrame);
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
    av_packet_free(&mixPacket);

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
