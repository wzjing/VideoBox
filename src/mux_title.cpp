#include "mux_title.h"
#include "utils/log.h"
#include "utils/snapshot.h"
#include "filter/blur_filter.h"

int error(const char *message, int ret) {
    LOGE("%s: %s\n", message, av_err2str(ret));
    return -1;
}

//static int64_t mux_duration = 0;

//int write(AVFormatContext *formatContext, AVCodecContext *context, AVStream *stream, AVPacket *packet) {
//    packet->stream_index = stream->index;
//    av_packet_rescale_ts(packet, context->time_base, stream->time_base);
//    if (av_compare_ts(packet->pts, stream->time_base, mux_duration, (AVRational) {1, 1}) > 0) {
//        LOGI("Stream #%d finished\n", stream->index);
//        return 1;
//    }
//
//    int ret = av_interleaved_write_frame(formatContext, packet);
//    if (ret != 0) {
//        LOGE("write frame error: %s\n", av_err2str(ret));
//        return 1;
//    }
//    return 0;
//}
//
//int
//encode(AVFormatContext *formatContext, AVCodecContext *context, AVStream *stream, AVFrame *frame) {
//    int ret = 0;
//    AVPacket packet{nullptr};
//    avcodec_send_frame(context, frame);
//    do {
//        ret = avcodec_receive_packet(context, &packet);
//        if (ret == 0) {
//            if (stream->index == 0) {
////                logPacket(&packet, "Video");
//            } else {
//                logPacket(&packet, "Audio");
//            }
//            if (packet.pts<0) return 1;
//            ret = write(formatContext, context, stream, &packet);
//            if (ret != 0) return 0;
//        } else if (ret == AVERROR(EAGAIN)) {
//            return 1;
//        } else {
//            LOGW("encode warning: %s\n", av_err2str(ret));
//            return 0;
//        }
//    } while (true);
//}

int mux_title(const char *input_filename, const char *output_filename) {
    AVFormatContext *inFormatContext = nullptr;
    AVFormatContext *outFormatContext = nullptr;
    AVCodec *vDecode = nullptr;
    AVCodec *aDecode = nullptr;
    AVCodec *vEncode = nullptr;
    AVCodec *aEncode = nullptr;
    AVCodecContext *vDecodeContext = nullptr;
    AVCodecContext *aDecodeContext = nullptr;
    AVCodecContext *vEncodeContext = nullptr;
    AVCodecContext *aEncodeContext = nullptr;
    AVStream *vInStream = nullptr;
    AVStream *aInStream = nullptr;
    AVStream *vOutStream = nullptr;
    AVStream *aOutStream = nullptr;
    int ret = 0;

    // open input file
    ret = avformat_open_input(&inFormatContext, input_filename, nullptr, nullptr);
    if (ret < 0) return error("avformat_open_input()", ret);
    ret = avformat_find_stream_info(inFormatContext, nullptr);
    if (ret < 0) return error("avformat_find_stream_info()", ret);

    ret = avformat_alloc_output_context2(&outFormatContext, nullptr, nullptr, output_filename);
    if (ret < 0) return error("avformat_alloc_output_context2()", ret);

    int i = 0;
    for (i = 0; i < inFormatContext->nb_streams; ++i) {
        if (!vInStream && inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vInStream = inFormatContext->streams[i];
            // create decoder
            vDecode = avcodec_find_decoder(vInStream->codecpar->codec_id);
            if (!vDecode) return error("video decoder avcodec_find_decoder", ret);
            vDecodeContext = avcodec_alloc_context3(vDecode);
            if (!vDecodeContext) return error("video decoder avcodec_alloc_context3", ret);
            ret = avcodec_parameters_to_context(vDecodeContext, vInStream->codecpar);
            if (ret < 0) return error("video decoder avcodec_parameters_to_context", ret);
            ret = avcodec_open2(vDecodeContext, vEncode, nullptr);
            if (ret < 0) return error("video decoder avcodec_open2", ret);

            // create encoder
            vEncode = avcodec_find_encoder(vInStream->codecpar->codec_id);
            if (!vEncode) return error("video encode avcodec_find_decoder", ret);
            vOutStream = avformat_new_stream(outFormatContext, vEncode);
            vOutStream->index = outFormatContext->nb_streams - 1;
            av_dict_copy(&vOutStream->metadata, vInStream->metadata, 0);
            vEncodeContext = avcodec_alloc_context3(vEncode);
            ret = avcodec_parameters_to_context(vEncodeContext, vInStream->codecpar);
            if (ret < 0) return error("video encode avcodec_parameters_to_context", ret);
            vEncodeContext->codec_id = vDecodeContext->codec_id;
            vEncodeContext->width = vDecodeContext->width;
            vEncodeContext->height = vDecodeContext->height;
            vEncodeContext->pix_fmt = vDecodeContext->pix_fmt;
            vEncodeContext->bit_rate = vDecodeContext->bit_rate;
            vEncodeContext->has_b_frames = vDecodeContext->has_b_frames;
            vEncodeContext->gop_size = 30;
            vEncodeContext->qmin = vDecodeContext->qmin;
            vEncodeContext->qmax = vDecodeContext->qmax;
            vEncodeContext->time_base = (AVRational) {vInStream->r_frame_rate.den, vInStream->r_frame_rate.num};
            vEncodeContext->profile = vDecodeContext->profile;
            vOutStream->time_base = vEncodeContext->time_base;

            AVDictionary *opt = nullptr;
            if (vEncodeContext->codec_id == AV_CODEC_ID_H264) {
                av_dict_set(&opt, "preset", "fast", 0);
                av_dict_set(&opt, "tune", "zerolatency", 0);
            }
            vOutStream->time_base = vEncodeContext->time_base;
            if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
                vEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            ret = avcodec_open2(vEncodeContext, vEncode, &opt);
            if (ret < 0) return error("video encode avcodec_open2", ret);
            avcodec_parameters_from_context(vOutStream->codecpar, vEncodeContext);
            if (ret < 0) return error("video encode avcodec_parameters_from_context", ret);


        } else if (!aInStream && inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            aInStream = inFormatContext->streams[i];
            // create decoder
            aDecode = avcodec_find_decoder(aInStream->codecpar->codec_id);
            if (!aDecode) return error("audio decoder avcodec_find_decoder", ret);
            aDecodeContext = avcodec_alloc_context3(aDecode);
            if (!aDecodeContext) return error("audio decoder avcodec_alloc_context3", ret);
            ret = avcodec_parameters_to_context(aDecodeContext, aInStream->codecpar);
            if (ret < 0) return error("audio decoder avcodec_parameters_to_context", ret);
            ret = avcodec_open2(aDecodeContext, aDecode, nullptr);
            if (ret < 0) return error("audio decoder avcodec_open2", ret);

            // create encoder
            aEncode = avcodec_find_encoder(aInStream->codecpar->codec_id);
            if (!aEncode) return error("audio encode avcodec_find_decoder", ret);
            aOutStream = avformat_new_stream(outFormatContext, aEncode);
            aOutStream->index = outFormatContext->nb_streams - 1;
            av_dict_copy(&aOutStream->metadata, aInStream->metadata, 0);
            aEncodeContext = avcodec_alloc_context3(aEncode);
            if (!aEncodeContext) return error("audio encode avcodec_alloc_context3", ret);
            ret = avcodec_parameters_to_context(aEncodeContext, aInStream->codecpar);
            if (ret < 0) return error("audio encode avcodec_parameters_to_context", ret);
            aEncodeContext->sample_fmt = aDecodeContext->sample_fmt;
            aEncodeContext->sample_rate = aDecodeContext->sample_rate;
            aEncodeContext->bit_rate = aDecodeContext->bit_rate;
            aEncodeContext->channel_layout = aDecodeContext->channel_layout;
            aEncodeContext->channels = aDecodeContext->channels;
            aEncodeContext->time_base = (AVRational) {1, aEncodeContext->sample_rate};
            aOutStream->time_base = aEncodeContext->time_base;
            if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
                aEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            ret = avcodec_open2(aEncodeContext, aEncode, nullptr);
            if (ret < 0) return error("audio encode avcodec_open2", ret);
            avcodec_parameters_from_context(aOutStream->codecpar, aEncodeContext);
            if (ret < 0) return error("audio encode avcodec_parameters_from_context", ret);

        }
    }

    logContext(vDecodeContext, "In", 1);
    logContext(aDecodeContext, "In", 0);
    logContext(vEncodeContext, "Out", 1);
    logContext(aEncodeContext, "Out", 0);

    logStream(vInStream, "In", 1);
    logStream(aInStream, "In", 0);
    logStream(vOutStream, "Out", 1);
    logStream(aOutStream, "Out", 0);

    if (!vDecodeContext || !aDecodeContext) return error("input file don't have video or audio track", -1);

    AVPacket *inPacket = av_packet_alloc();
    AVFrame *inVideoFrame = av_frame_alloc();
    AVFrame *inAudioFrame = av_frame_alloc();

    int got_audio = 0;
    int got_video = 0;

    // Get the first audio frame and video frame
    while (!got_audio || !got_video) {
        ret = av_read_frame(inFormatContext, inPacket);
        if (ret != 0) {
            LOGE("unable to read frame from input");
            break;
        }
        if (!got_video && inPacket->stream_index == vInStream->index) {
            avcodec_send_packet(vDecodeContext, inPacket);
            if (avcodec_receive_frame(vDecodeContext, inVideoFrame) == 0) {
                logFrame(inVideoFrame, "Source", 1);
                got_video = 1;
            }
        } else if (!got_audio && inPacket->stream_index == aInStream->index) {
            avcodec_send_packet(aDecodeContext, inPacket);
            if (avcodec_receive_frame(aDecodeContext, inAudioFrame) == 0) {
                logFrame(inAudioFrame, "Source", 0);
                got_audio = 1;
            }
        }
    }

    // blur the first frame
    BlurFilter blurFilter(vDecodeContext->width, vDecodeContext->height, vDecodeContext->pix_fmt, 30, 6, "Title");
    blurFilter.init();
    // free input context
    av_packet_free(&inPacket);
    avformat_free_context(inFormatContext);
    avcodec_free_context(&vDecodeContext);
    avcodec_free_context(&aDecodeContext);

    if (!(outFormatContext->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", output_filename);
        ret = avio_open(&outFormatContext->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
            return -1;
        }
    }

    ret = avformat_write_header(outFormatContext, nullptr);
    if (ret < 0) {
        LOGE("error occurred when opening output file %s", av_err2str(ret));
        return -1;
    }

    // Encode output
    AVFrame *outVideoFrame = av_frame_alloc();
    AVFrame *outAudioFrame = av_frame_alloc();
    AVPacket *outPacket = av_packet_alloc();

    int64_t video_pts = 0;
    int64_t audio_pts = 0;

    int duration = 2;

    int encode_video = 1;
    int encode_audio = 1;

    // erase audio frame data
    int sample_size = av_get_bytes_per_sample((AVSampleFormat) inAudioFrame->format);
    for (i = 0; i < inAudioFrame->channels; i++) {
        memset(inAudioFrame->data[i], '0', inAudioFrame->nb_samples * sample_size);
    }

    outVideoFrame->width = inVideoFrame->width;
    outVideoFrame->height = inVideoFrame->height;
    outVideoFrame->format = inVideoFrame->format;
    av_frame_get_buffer(outVideoFrame, 0);
    outAudioFrame->format = inAudioFrame->format;
    outAudioFrame->nb_samples = inAudioFrame->nb_samples;
    outAudioFrame->sample_rate = inAudioFrame->sample_rate;
    outAudioFrame->channel_layout = inAudioFrame->channel_layout;
    av_frame_get_buffer(outAudioFrame, 0);

    i = 0;
    int index = 0;
    while (encode_video || encode_audio) {
        if (!encode_audio || (encode_video && av_compare_ts(video_pts, vEncodeContext->time_base,
                                                            audio_pts, aEncodeContext->time_base) <= 0)) {
            LOGD("Video Time: %s(%ld-%d/%d) ratio: %f\n",
                 av_ts2timestr(video_pts, &vEncodeContext->time_base),
                 video_pts,
                 vEncodeContext->time_base.num,
                 vEncodeContext->time_base.den,
                 av_q2d(vEncodeContext->time_base));
            if (av_compare_ts(video_pts, vEncodeContext->time_base,
                              duration, (AVRational) {1, 1}) > 0 || index>=59) {
                outVideoFrame = nullptr;
            } else {
                index++;
                i++;
                float blur = 30.0 - i > 0 ? 30 - i : 0;
                blurFilter.setConfig(blur, 6);
                blurFilter.init();
                av_frame_copy(outVideoFrame, inVideoFrame);
                av_frame_copy_props(outVideoFrame, inVideoFrame);
                outVideoFrame->pict_type = AV_PICTURE_TYPE_NONE;
                ret = blurFilter.filter(outVideoFrame);
                if (ret < 0) goto error;
                blurFilter.destroy();
                outVideoFrame->pts = video_pts;
                video_pts +=1; // vEncodeContext->time_base.num;
                logFrame(outVideoFrame, "Out", 1);
            }
            avcodec_send_frame(vEncodeContext, outVideoFrame);
            while (true) {
                ret = avcodec_receive_packet(vEncodeContext, outPacket);
                if (ret == 0) {
                    av_packet_rescale_ts(outPacket, vEncodeContext->time_base, vOutStream->time_base);
                    outPacket->stream_index = vOutStream->index;
                    logPacket(outPacket, "V");
                    ret = av_interleaved_write_frame(outFormatContext, outPacket);
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
            if (av_compare_ts(audio_pts, aEncodeContext->time_base,
                              duration, (AVRational) {1, 1}) >= 0) {
                inAudioFrame = nullptr;
            } else {
                inAudioFrame->pts = audio_pts;
                audio_pts += inAudioFrame->nb_samples;
            }
            avcodec_send_frame(aEncodeContext, inAudioFrame);
            while (true) {
                ret = avcodec_receive_packet(aEncodeContext, outPacket);
                if (ret == 0) {
                    av_packet_rescale_ts(outPacket, aEncodeContext->time_base, aOutStream->time_base);
                    outPacket->stream_index = aOutStream->index;
//                    logPacket(outPacket, "A");
                    ret = av_interleaved_write_frame(outFormatContext, outPacket);
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

    avcodec_flush_buffers(vEncodeContext);
    avcodec_flush_buffers(aEncodeContext);

    av_write_trailer(outFormatContext);
    LOGD("trailer write\n");

    if (!(outFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outFormatContext->pb);
    }

    av_frame_free(&inVideoFrame);
    av_frame_free(&inAudioFrame);
    av_packet_free(&outPacket);
    avformat_free_context(outFormatContext);
    avcodec_free_context(&vEncodeContext);
    avcodec_free_context(&aEncodeContext);

    return 0;
    error:
    av_frame_free(&inVideoFrame);
    av_frame_free(&inAudioFrame);
    av_packet_free(&outPacket);
    avformat_free_context(outFormatContext);
    avcodec_free_context(&vEncodeContext);
    avcodec_free_context(&aEncodeContext);
    return -1;
}
