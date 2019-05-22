#include "./utils/log.h"
#include "./utils/error.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
}

#include "concat_add_title.h"

#define TITLE_DURATION 2

int encode_title(AVFormatContext *formatContext,
                 AVFrame *srcVideoFrame, AVFrame *srcAudioFrame,
                 AVCodecContext *videoCodecContext, AVCodecContext *audioCodecContext,
                 AVStream *videoStream, AVStream *audioStream,
                 int64_t &video_pts, int64_t &audio_pts,
                 int64_t &video_dts, int64_t &audio_dts) {

    int encode_video = 1;
    int encode_audio = 1;
    int ret = 0;

    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    videoFrame->width = videoCodecContext->width;
    videoFrame->height = videoCodecContext->height;
    videoFrame->format = videoCodecContext->pix_fmt;
    av_frame_get_buffer(videoFrame, 0);

    audioFrame->format = audioCodecContext->sample_fmt;
    audioFrame->nb_samples = srcAudioFrame->nb_samples;
    audioFrame->sample_rate = audioCodecContext->sample_rate;
    audioFrame->channel_layout = audioCodecContext->channel_layout;
    av_frame_get_buffer(audioFrame, 0);

    int64_t video_frame_pts = 0;
    int64_t audio_frame_pts = 0;

    uint64_t next_video_pts = 0;
    uint64_t next_video_dts = 0;
    uint64_t next_audio_pts = 0;
    uint64_t next_audio_dts = 0;

    while (encode_video || encode_audio) {
        if (!encode_audio || (encode_video && av_compare_ts(video_pts, videoCodecContext->time_base,
                                                            audio_pts, audioCodecContext->time_base) <= 0)) {
            LOGD("Video Time: %s(%ld-%d/%d) ratio: %f\n",
                 av_ts2timestr(video_pts, &videoCodecContext->time_base),
                 video_pts,
                 videoCodecContext->time_base.num,
                 videoCodecContext->time_base.den,
                 av_q2d(videoCodecContext->time_base));
            if (av_compare_ts(video_pts, videoCodecContext->time_base,
                              TITLE_DURATION, (AVRational) {1, 1}) > 0) {
                videoFrame = nullptr;
            } else {
                av_frame_copy(videoFrame, srcVideoFrame);
                av_frame_copy_props(videoFrame, srcAudioFrame);
                videoFrame->pict_type = AV_PICTURE_TYPE_NONE;
                videoFrame->pts = video_frame_pts;
                video_frame_pts += 1;
                logFrame(videoFrame, "Out", 1);
            }
            avcodec_send_frame(videoCodecContext, videoFrame);
            while (true) {
                ret = avcodec_receive_packet(videoCodecContext, packet);
                if (ret == 0) {
                    av_packet_rescale_ts(packet, videoCodecContext->time_base, videoStream->time_base);
                    packet->stream_index = videoStream->index;
                    packet->pts += video_pts;
                    packet->dts += video_dts;
                    next_video_pts = packet->dts + packet->duration;
                    next_video_dts = packet->dts + packet->duration;
                    logPacket(packet, "V");
                    ret = av_interleaved_write_frame(formatContext, packet);
                    if (ret < 0) {
                        LOGE("write video frame error: %s\n", av_err2str(ret));
                        goto error;
                    }
                } else if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                    LOGW("Stream Video finished\n");
                    encode_video = 0;
                    break;
                } else {
                    LOGE("encode video frame error: %s\n", av_err2str(ret));
                    goto error;
                }
            }
        } else {
            if (av_compare_ts(audio_pts, audioCodecContext->time_base,
                              TITLE_DURATION, (AVRational) {1, 1}) >= 0) {
                audioFrame = nullptr;
            } else {
                audioFrame->pts = audio_frame_pts;
                audio_frame_pts += audioFrame->nb_samples;
            }
            avcodec_send_frame(audioCodecContext, audioFrame);
            while (true) {
                ret = avcodec_receive_packet(audioCodecContext, packet);
                if (ret == 0) {
                    av_packet_rescale_ts(packet, audioCodecContext->time_base, audioStream->time_base);
                    packet->stream_index = audioStream->index;
                    packet->pts += audio_pts;
                    packet->dts += audio_dts;
                    next_audio_pts = packet->pts + packet->duration;
                    next_audio_dts = packet->dts + packet->duration;
                    logPacket(packet, "A");
                    ret = av_interleaved_write_frame(formatContext, packet);
                    if (ret < 0) {
                        LOGE("write audio frame error: %s\n", av_err2str(ret));
                        goto error;
                    }
                    break;
                } else if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret == AVERROR_EOF) {
                    LOGW("Stream Audio finished\n");
                    encode_audio = 0;
                    break;
                } else {
                    LOGE("encode audio frame error: %s\n", av_err2str(ret));
                    goto error;
                }
            }
        }
    }

    video_pts = next_video_pts;
    video_dts = next_video_dts;
    audio_pts = next_audio_pts;
    audio_dts = next_audio_dts;

    av_frame_free(&audioFrame);
    av_frame_free(&videoFrame);
    av_packet_free(&packet);
    return 0;
    error:
    av_frame_free(&audioFrame);
    av_frame_free(&videoFrame);
    av_packet_free(&packet);
    return -1;
}

int concat_add_title(const char *output_filename, char **input_filenames, size_t nb_inputs) {

    int ret = 0;
    // input fragments
    auto **videos = (Video **) malloc(nb_inputs * sizeof(Video *));

    for (int i = 0; i < nb_inputs; ++i) {
        videos[i] = (Video *) malloc(sizeof(Video));
    }

    // output video
    AVFormatContext *outFmtContext = nullptr;
    AVStream *outVideoStream = nullptr;
    AVStream *outAudioStream = nullptr;
    AVCodecContext *outVideoContext = nullptr;
    AVCodecContext *outAudioContext = nullptr;
    AVCodec *outVideoCodec = nullptr;
    AVCodec *outAudioCodec = nullptr;

    for (int i = 0; i < nb_inputs; ++i) {
        ret = avformat_open_input(&videos[i]->formatContext, input_filenames[i], nullptr, nullptr);
        if (ret < 0) return error(ret, "input format error");
        ret = avformat_find_stream_info(videos[i]->formatContext, nullptr);
        if (ret < 0) return error(ret, "input format info error");
        for (int j = 0; j < videos[i]->formatContext->nb_streams; ++j) {
            if (videos[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videos[i]->videoStream = videos[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(videos[i]->videoStream->codecpar->codec_id);
                videos[i]->videoCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(videos[i]->videoCodecContext, videos[i]->videoStream->codecpar);
                avcodec_open2(videos[i]->videoCodecContext, codec, nullptr);
            } else if (videos[i]->formatContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                videos[i]->audioStream = videos[i]->formatContext->streams[j];
                AVCodec *codec = avcodec_find_decoder(videos[i]->audioStream->codecpar->codec_id);
                videos[i]->audioCodecContext = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(videos[i]->audioCodecContext, videos[i]->audioStream->codecpar);
                avcodec_open2(videos[i]->audioCodecContext, codec, nullptr);
            }
            if (videos[i]->videoStream && videos[i]->audioStream) break;
        }
        LOGD("\n");
        videos[i]->videoStream ? LOGD("Video/") : LOGD("--/");
        videos[i]->audioStream ? LOGD("Audio") : LOGD("--");
        LOGD(": %s\n", input_filenames[i]);
        LOGD("\n");
    }

    // create output AVFormatContext
    ret = avformat_alloc_output_context2(&outFmtContext, nullptr, nullptr, output_filename);
    if (ret < 0) return error(ret, "output format error");

    // Copy codec from input video AVStream
    outVideoCodec = avcodec_find_encoder(videos[0]->videoStream->codecpar->codec_id);
    outAudioCodec = avcodec_find_encoder(videos[0]->audioStream->codecpar->codec_id);

    // create output Video AVStream
    outVideoStream = avformat_new_stream(outFmtContext, outVideoCodec);
    if (!outVideoStream) return error(ret, "create output video stream error");
    outVideoStream->id = 0;

    outAudioStream = avformat_new_stream(outFmtContext, outAudioCodec);
    if (!outAudioStream) return error(ret, "create output audio stream error");
    outAudioStream->id = 1;

    Video *baseVideo = videos[0]; // the result code is based on this video

    // Copy Video Stream Configure from base Video
    outVideoContext = avcodec_alloc_context3(outVideoCodec);
    outVideoContext->codec_id = baseVideo->videoCodecContext->codec_id;
    outVideoContext->width = baseVideo->videoCodecContext->width;
    outVideoContext->height = baseVideo->videoCodecContext->height;
    outVideoContext->pix_fmt = baseVideo->videoCodecContext->pix_fmt;
    outVideoContext->bit_rate = baseVideo->videoCodecContext->bit_rate;
    outVideoContext->has_b_frames = baseVideo->videoCodecContext->has_b_frames;
    outVideoContext->gop_size = baseVideo->videoCodecContext->gop_size;
    outVideoContext->qmin = baseVideo->videoCodecContext->qmin;
    outVideoContext->qmax = baseVideo->videoCodecContext->qmax;
    outVideoContext->time_base = (AVRational) {baseVideo->videoStream->r_frame_rate.den,
                                               baseVideo->videoStream->r_frame_rate.num};
    outVideoContext->profile = baseVideo->videoCodecContext->profile;
    outVideoStream->time_base = outVideoContext->time_base;
    ret = avcodec_open2(outVideoContext, outVideoCodec, nullptr);
    if (ret < 0) return error(ret, "Open Video output AVCodecContext");
    ret = avcodec_parameters_from_context(outVideoStream->codecpar, outVideoContext);
    if (ret < 0) return error(ret, "Copy Video Context to output stream");

    // Copy Audio Stream Configure from base Video
    outAudioContext = avcodec_alloc_context3(outAudioCodec);
    outAudioContext->codec_type = baseVideo->audioCodecContext->codec_type;
    outAudioContext->codec_id = baseVideo->audioCodecContext->codec_id;
    outAudioContext->sample_fmt = baseVideo->audioCodecContext->sample_fmt;
    outAudioContext->sample_rate = baseVideo->audioCodecContext->sample_rate;
    outAudioContext->bit_rate = baseVideo->audioCodecContext->bit_rate;
    outAudioContext->channel_layout = baseVideo->audioCodecContext->channel_layout;
    outAudioContext->channels = baseVideo->audioCodecContext->channels;
    outAudioContext->time_base = (AVRational) {1, outAudioContext->sample_rate};
    outAudioStream->time_base = outAudioContext->time_base;
    ret = avcodec_open2(outAudioContext, outAudioCodec, nullptr);
    if (ret < 0) return error(ret, "Open Audio output AVCodecContext");
    ret = avcodec_parameters_from_context(outAudioStream->codecpar, outAudioContext);
    if (ret < 0) return error(ret, "Copy Audio Context to output stream");

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


    av_dump_format(outFmtContext, 0, output_filename, 1);

    AVPacket *packet = av_packet_alloc();
    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();

    uint64_t last_video_pts = 0;
    uint64_t last_video_dts = 0;
    uint64_t last_audio_pts = 0;
    uint64_t last_audio_dts = 0;


    for (int i = 0; i < nb_inputs; ++i) {
        AVFormatContext *inFormatContext = videos[i]->formatContext;

        AVStream *inVideoStream = videos[i]->videoStream;
        AVStream *inAudioStream = videos[i]->audioStream;
        AVCodecContext *audioContext = videos[i]->audioCodecContext;
        AVCodecContext *videoContext = videos[i]->videoCodecContext;

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


        // use first frame to make a title
        av_frame_unref(videoFrame);
        av_frame_unref(audioFrame);
        int got_video = 0;
        int got_audio = 0;
        do {
            ret = av_read_frame(inFormatContext, packet);
            if (ret < 0) break;
            if (!got_video && packet->stream_index == inVideoStream->index) {
                ret = avcodec_send_packet(videoContext, packet);
                if (ret < 0) continue;
                ret = avcodec_receive_frame(videoContext, videoFrame);
                if (ret < 0) continue;
                else got_video = 1;
            } else if (!got_audio && packet->stream_index == inAudioStream->index) {
                ret = avcodec_send_packet(audioContext, packet);
                if (ret < 0) continue;
                ret = avcodec_receive_frame(audioContext, audioFrame);
                if (ret < 0) continue;
                else got_audio = 1;
            }
        } while (!got_video || !got_audio);

        if (!got_video) {
            videoFrame->width = outVideoContext->width;
            videoFrame->height = outVideoContext->height;
            videoFrame->format = outVideoContext->pix_fmt;
            av_frame_get_buffer(videoFrame, 0);
        }

        if (!got_audio) {
            audioFrame->nb_samples = 1024;
            audioFrame->sample_rate = outAudioContext->sample_rate;
        }


        avio_seek(inFormatContext->pb, 0, SEEK_SET);
        // copy the video file
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
                if (packet->flags & AV_PKT_FLAG_DISCARD) {
                    LOGW("\nPacket is discard\n");
                    continue;
                }

                if (!video_ts_set) {
                    first_video_pts = packet->pts;
                    first_video_dts = packet->dts;
                    video_ts_set = 1;
                }

                packet->stream_index = outVideoStream->index;

                packet->pts -= first_video_pts;
                packet->dts -= first_video_dts;
                av_packet_rescale_ts(packet, inVideoStream->time_base, outVideoStream->time_base);
                packet->pts += last_video_pts;
                packet->dts += last_video_dts;
                next_video_pts = packet->pts + packet->duration;
                next_video_dts = packet->dts + packet->duration;
                LOGD("\033[32mVIDEO\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\tKEY:%s\n",
                     old_pts, old_dts,
                     packet->pts,
                     packet->dts,
                     packet->flags & AV_PKT_FLAG_KEY ? "Key" : "--");
                av_interleaved_write_frame(outFmtContext, packet);
            } else if (packet->stream_index == inAudioStream->index) {
                logPacket(packet, "origin");
                int64_t old_pts = packet->pts;
                int64_t old_dts = packet->dts;

                packet->stream_index = outAudioStream->index;

                if (!audio_ts_set) {
                    first_audio_pts = packet->pts;
                    first_audio_dts = packet->dts;
                    audio_ts_set = 1;
                }
                packet->pts -= first_audio_pts;
                packet->dts -= first_audio_dts;
                av_packet_rescale_ts(packet, inAudioStream->time_base, outAudioStream->time_base);
                packet->pts += last_audio_pts;
                packet->dts += last_audio_dts;
                next_audio_pts = packet->pts + packet->duration;
                next_audio_dts = packet->dts + packet->duration;
                LOGD("\033[32mAudio\033[0m: PTS: %-8ld\tDTS: %-8ld -> PTS: %-8ld\tDTS: %-8ld\n", old_pts, old_dts,
                     packet->pts,
                     packet->dts);
                av_interleaved_write_frame(outFmtContext, packet);
            }
        } while (true);
        last_video_pts = next_video_pts;
        last_video_dts = next_video_dts;
        last_audio_pts = next_audio_pts;
        last_audio_dts = next_audio_dts;

        LOGD("--------------------------------------------------------------------------------\n");
    }


    av_write_trailer(outFmtContext);


    if (!(outFmtContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFmtContext->pb);
    }


    av_packet_free(&packet);
    avformat_free_context(outFmtContext);

    for (int i = 0; i < nb_inputs; ++i) {
        avformat_free_context(videos[i]->formatContext);
        if (videos[i]->videoCodecContext)
            avcodec_free_context(&videos[i]->videoCodecContext);
        if (videos[i]->audioCodecContext)
            avcodec_free_context(&videos[i]->audioCodecContext);
        free(videos[i]);
    }
    free(videos);

    return 0;
}
