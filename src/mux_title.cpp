#include "mux_title.h"
#include "utils/log.h"
#include "utils/snapshot.h"
#include "filter/blur_filter.h"

int error(const char *message, int ret) {
    LOGE("%s: %s\n", message, av_err2str(ret));
    return -1;
}

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

    ret = avformat_alloc_output_context2(&outFormatContext, nullptr, "mp4", output_filename);
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
            vEncodeContext = avcodec_alloc_context3(vEncode);
            if (!vEncodeContext) return error("video encode avcodec_alloc_context3", ret);
            ret = avcodec_parameters_to_context(vEncodeContext, vInStream->codecpar);
            if (ret < 0) return error("video encode avcodec_parameters_to_context", ret);
            vEncodeContext->time_base = (AVRational) {vInStream->r_frame_rate.den, vInStream->r_frame_rate.num};
            vEncodeContext->framerate = vInStream->r_frame_rate;
            vEncodeContext->gop_size = 12;
            vEncodeContext->has_b_frames = 1;
            vEncodeContext->max_b_frames = 10;
            vEncodeContext->qmin = 10;
            vEncodeContext->qmax = 50;

            AVDictionary * opt= nullptr;
            if(vEncodeContext->codec_id == AV_CODEC_ID_H264) {
                av_dict_set(&opt, "preset", "slow", 0);
                av_dict_set(&opt, "tune", "zerolatency", 0);
                av_dict_set(&opt, "profile", "main", 0);
            }
            vOutStream->time_base = vEncodeContext->time_base;
            ret = avcodec_open2(vEncodeContext, vEncode, &opt);
            if (ret < 0) return error("video encode avcodec_open2", ret);
            avcodec_parameters_from_context(vOutStream->codecpar, vEncodeContext);
            if (ret < 0) return error("video encode avcodec_parameters_from_context", ret);

            if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
                vEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
            aEncodeContext = avcodec_alloc_context3(aEncode);
            if (!aEncodeContext) return error("audio encode avcodec_alloc_context3", ret);
            ret = avcodec_parameters_to_context(aEncodeContext, aInStream->codecpar);
            if (ret < 0) return error("audio encode avcodec_parameters_to_context", ret);
            aEncodeContext->time_base = (AVRational) {1, aEncodeContext->sample_rate};
            aOutStream->time_base = aEncodeContext->time_base;
            ret = avcodec_open2(aEncodeContext, aEncode, nullptr);
            if (ret < 0) return error("audio encode avcodec_open2", ret);
            avcodec_parameters_from_context(aOutStream->codecpar, aEncodeContext);
            if (ret < 0) return error("audio encode avcodec_parameters_from_context", ret);

            if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
                aEncodeContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    logContext(vEncodeContext, "H264", 1);
    logContext(aEncodeContext, "AAC", 0);

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
                got_video = 1;
            }
        } else if (!got_audio && inPacket->stream_index == aInStream->index) {
            avcodec_send_packet(aDecodeContext, inPacket);
            if (avcodec_receive_frame(aDecodeContext, inAudioFrame) == 0) {
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
    AVFrame *outVideoFrame = nullptr;
    AVPacket *outPacket = av_packet_alloc();

    int64_t video_pts = 0;
    int64_t audio_pts = 0;

    int duration = 2;

    int encode_video = 1;
    int encode_audio = 1;

    // erase audio frame data
    int sample_size = av_get_bytes_per_sample((AVSampleFormat) inAudioFrame->format);
    for (i = 0; i < inAudioFrame->channels; i++) {
        LOGD("clean %d/%d\n", i, inAudioFrame->linesize[i]);
        memset(inAudioFrame->data[i], '0', inAudioFrame->nb_samples * sample_size);
    }

    i = 0;
    while (encode_video || encode_audio) {
        if (!encode_audio || av_compare_ts(video_pts, vEncodeContext->time_base,
                                                            audio_pts, aEncodeContext->time_base) <= 0) {

            if (av_compare_ts(video_pts, vEncodeContext->time_base,
                              duration, (AVRational) {1, 1}) > 0) {
                outVideoFrame = nullptr;
            } else {
                i++;
                float blur = 30.0 - i > 0 ? 30 - i : 0;
                LOGD("blur: %f\n", blur);
                blurFilter.setConfig(blur, 6);
                blurFilter.init();
                outVideoFrame = av_frame_clone(inVideoFrame);
                blurFilter.filter(outVideoFrame);
                blurFilter.destroy();
                if (!outVideoFrame) {
                    LOGE("unable to clone video frame\n");
                    goto error;
                }
                outVideoFrame->pts = video_pts;
                video_pts++;
            }
            avcodec_send_frame(vEncodeContext, outVideoFrame);
            LOGD("send video frame\n");
            while (true) {
                ret = avcodec_receive_packet(vEncodeContext, outPacket);
                if (ret == 0) {
                    LOGD("received video packet: %ld\n", outPacket->pts);
                    av_packet_rescale_ts(outPacket, vEncodeContext->time_base, vOutStream->time_base);
                    outPacket->stream_index = vOutStream->index;
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
            av_frame_unref(outVideoFrame);
        } else {
            if (av_compare_ts(audio_pts, aEncodeContext->time_base,
                              duration, (AVRational) {1, 1}) >= 0) {
                inAudioFrame = nullptr;
            } else {
                inAudioFrame->pts = audio_pts;
                audio_pts += inAudioFrame->nb_samples;
            }
            avcodec_send_frame(aEncodeContext, inAudioFrame);
            LOGD("send audio frame\n");
            while (true) {
                ret = avcodec_receive_packet(aEncodeContext, outPacket);
                if (ret == 0) {
                    LOGD("received audio packet: %ld\n", outPacket->pts);
                    av_packet_rescale_ts(outPacket, aEncodeContext->time_base, aOutStream->time_base);
                    outPacket->stream_index = aOutStream->index;
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
