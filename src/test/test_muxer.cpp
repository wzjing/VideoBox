#include "test_muxer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int test_muxer(const char *output_filename, const char *input_video, const char *input_audio) {
    Muxer *muxer = create_muxer(output_filename);
    AVDictionary *video_opt = nullptr;
//    av_dict_set(&video_opt, "preset", "fast", 0);
    MediaConfig video_config;
    video_config.media_type = AVMEDIA_TYPE_VIDEO;
    video_config.codec_id = AV_CODEC_ID_H264;
    video_config.width = 1920;
    video_config.height = 1080;
    video_config.bit_rate = 7200000;
    video_config.format = AV_PIX_FMT_YUV420P;
    video_config.frame_rate = 30;
    video_config.gop_size = 12;
    Media *video = add_media(muxer, &video_config, video_opt);
    av_dict_free(&video_opt);
    MediaConfig audio_config;
    audio_config.media_type = AVMEDIA_TYPE_AUDIO;
    audio_config.codec_id = AV_CODEC_ID_AAC;
    audio_config.format = AV_SAMPLE_FMT_FLTP;
    audio_config.bit_rate = 64000;
    audio_config.sample_rate = 48000;
    audio_config.nb_samples = 1024;
    audio_config.channel_layout = AV_CH_LAYOUT_STEREO;
    Media *audio = add_media(muxer, &audio_config, nullptr);

    FILE *v_file = fopen(input_video, "rb");
    FILE *a_file = fopen(input_audio, "rb");
    mux(muxer, [&video, &audio, &v_file, &a_file](AVFrame *frame, int type) -> int {
        if (type == AVMEDIA_TYPE_VIDEO) {
            return read_yuv(v_file, frame, frame->width, frame->height, video->codec_ctx->frame_number,
                            (AVPixelFormat) frame->format);
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            return read_pcm(a_file, frame, frame->nb_samples, frame->channels, audio->codec_ctx->frame_number,
                            (AVSampleFormat) frame->format);
        } else {
            return 1;
        }
    }, nullptr);


    close_muxer(muxer);
    return 0;
}

static int64_t mux_duration = 0;

int write(AVFormatContext *formatContext, AVCodecContext *context, AVStream *stream, AVPacket *packet) {
    packet->stream_index = stream->index;
    av_packet_rescale_ts(packet, context->time_base, stream->time_base);
    if (av_compare_ts(packet->pts, stream->time_base, mux_duration, (AVRational) {1, 1}) > 0) {
        LOGI("Stream #%d finished\n", stream->index);
        return 1;
    }

    int ret = av_interleaved_write_frame(formatContext, packet);
    if (ret != 0) {
        LOGE("write frame error: %s\n", av_err2str(ret));
        return 1;
    }
    return 0;
}

int
encode(AVFormatContext *formatContext, AVCodecContext *context, AVStream *stream, AVFrame *frame) {
    int ret = 0;
    AVPacket packet{nullptr};
    avcodec_send_frame(context, frame);
    do {
        ret = avcodec_receive_packet(context, &packet);
        if (ret == 0) {
            if (stream->index == 0) {
//                logPacket(&packet, "Video");
            } else {
                logPacket(&packet, "Audio");
            }
            if (packet.pts<0) return 1;
            ret = write(formatContext, context, stream, &packet);
            if (ret != 0) return 0;
        } else if (ret == AVERROR(EAGAIN)) {
            return 1;
        } else {
            LOGW("encode warning: %s\n", av_err2str(ret));
            return 0;
        }
    } while (true);
}

void fillVideoFrame(AVCodecContext *context, AVFrame *frame, FILE *file) {
    read_yuv(file, frame, context->width, context->height, context->frame_number, context->pix_fmt);
}

void fillAudioFrame(AVCodecContext *context, AVFrame *frame, FILE *file) {
    read_pcm(file, frame, 1024, context->channels, context->frame_number, context->sample_fmt);
}

int test_mux(const char *output_filename, const char *input_video, const char *input_audio, int64_t duration) {
    mux_duration = duration;
    AVFormatContext *formatContext;
    AVCodec *videoCodec;
    AVCodec *audioCodec;
    AVCodecContext *videoContext = nullptr;
    AVCodecContext *audioContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;

    int ret = 0;
    avformat_alloc_output_context2(&formatContext, nullptr, nullptr, output_filename);

    if (!formatContext) {
        LOGE("unable to create AVFormatContext\n");
        exit(1);
    }
    videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);

    videoStream = avformat_new_stream(formatContext, nullptr);
    videoStream->index = formatContext->nb_streams - 1;
    audioStream = avformat_new_stream(formatContext, nullptr);
    audioStream->index = formatContext->nb_streams - 1;

    videoContext = avcodec_alloc_context3(videoCodec);
    videoContext->codec_id = AV_CODEC_ID_H264;
    videoContext->width = 1920;
    videoContext->height = 1080;
    videoContext->pix_fmt = AV_PIX_FMT_YUV420P;
    videoContext->bit_rate = 7000000;
    videoStream->time_base = (AVRational) {1, 30};
    videoContext->time_base = videoStream->time_base;
    videoContext->gop_size = 12;
    videoContext->has_b_frames = 0;
    AVDictionary * opt = nullptr;
//    av_dict_set(&opt, "subq", "6", 0);
    av_dict_set(&opt, "preset", "superfast", 0);
    av_dict_set(&opt, "tune", "zerolatency", 0);
    av_dict_set(&opt, "profile", "main", 0);
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        videoContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_open2(videoContext, videoCodec, &opt);
    avcodec_parameters_from_context(videoStream->codecpar, videoContext);

    audioContext = avcodec_alloc_context3(audioCodec);
    audioContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
    audioContext->bit_rate = 192000;
    audioContext->sample_rate = 44100;
    audioContext->channel_layout = AV_CH_LAYOUT_STEREO;
    audioContext->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    audioStream->time_base = (AVRational) {1, 44100};
    audioContext->time_base = audioStream->time_base;
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        audioContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_open2(audioContext, audioCodec, nullptr);
    avcodec_parameters_from_context(audioStream->codecpar, audioContext);


    av_dump_format(formatContext, 0, output_filename, 1);

    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", output_filename);
        ret = avio_open(&formatContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(formatContext, &opt);
    if (ret < 0) {
        LOGE("error occurred when opening output file %s", av_err2str(ret));
        return -1;
    }

    FILE *video_file = fopen(input_video, "rb");
    FILE *audio_file = fopen(input_audio, "rb");

    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();
    videoFrame->format = videoContext->pix_fmt;
    videoFrame->width = videoContext->width;
    videoFrame->height = videoContext->height;
    av_frame_get_buffer(videoFrame, 0);
    audioFrame->format = audioContext->sample_fmt;
    audioFrame->nb_samples = 1024;
    audioFrame->sample_rate = audioContext->sample_rate;
    audioFrame->channel_layout = audioContext->channel_layout;
    av_frame_get_buffer(audioFrame, 0);

    int encode_video = 1;
    int encode_audio = 1;
    int64_t next_video_pts = 0;
    int64_t next_audio_pts = 0;

    while (encode_video || encode_audio) {
        if (!encode_audio || av_compare_ts(next_video_pts,
                                           videoContext->time_base,
                                           next_audio_pts,
                                           audioContext->time_base) <= 0) {
            fillVideoFrame(videoContext, videoFrame, video_file);
            videoFrame->pts = next_video_pts;
            next_video_pts++;
            encode_video = encode(formatContext, videoContext, videoStream, videoFrame);
        } else {
            fillAudioFrame(audioContext, audioFrame, audio_file);
            audioFrame->pts = next_audio_pts;
            next_audio_pts += 1024;
            encode_audio = encode(formatContext, audioContext, audioStream, audioFrame);
        }
    }

    av_write_trailer(formatContext);

    avcodec_free_context(&videoContext);
    avcodec_free_context(&audioContext);
    avformat_free_context(formatContext);
    return 0;
}

int test_remux(const char *in_filename, const char *out_filename) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = (int*)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

    while (1) {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        logPacket(&pkt, "in");

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_DOWN|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_DOWN|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        logPacket(&pkt, "out");

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);
    end:

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
