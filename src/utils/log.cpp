//
// Created by android1 on 2019/5/16.
//

#include "log.h"

void logContext(AVCodecContext *context, const char *tag, int isVideo) {
    if (isVideo) {
        LOGD("Video Context(\033[33m%s\033[0m)->\tcodec: %s\tpix:%s\tsize: %dx%d\tframerate{%d, %d}\tgroup: %d\tmax_b: %d\ttimebase{%d, %d}\n",
             tag,
             avcodec_get_name(context->codec_id),
             av_get_pix_fmt_name(context->pix_fmt),
             context->width,
             context->height,
             context->framerate.num,
             context->framerate.den,
             context->gop_size,
             context->max_b_frames,
             context->time_base.num,
             context->time_base.den);
    } else {
        LOGD("Audio Context(\033[33m%s\033[0m)->\tcodec: %s\tsample:%s\tsample_rate: %d\tbitrate: %ld\ttimebase{%d, %d}\n",
             tag,
             avcodec_get_name(context->codec_id),
             av_get_sample_fmt_name(context->sample_fmt),
             context->sample_rate,
             context->bit_rate,
             context->time_base.num,
             context->time_base.den);
    }
}

void logPacket(AVPacket *packet, const char *tag) {
    LOGD("packet(\033[32m%s\033[0m)->\tstream: %d\tsize:%d\tPTS: %ld\tDTS: %ld\n", tag, packet->stream_index,
         packet->size,
         packet->dts, packet->pts);
}

void logFrame(AVFrame *frame, const char *tag, int isVideo) {
    if (isVideo) {
        const char *type;
        switch (frame->pict_type) {
            case AV_PICTURE_TYPE_I:
                type = "I";
                break;
            case AV_PICTURE_TYPE_B:
                type = "B";
                break;
            case AV_PICTURE_TYPE_P:
                type = "P";
                break;
            default:
                type = "--";
                break;
        }
        LOGD("video frame(\033[33m%s\033[0m)-> type:%s\tfmt:%d\tsize: %dx%d\tPTS: %8ld\n",
             tag,
             type,
             frame->format,
             frame->width,
             frame->height,
             frame->pts);
    } else {
        LOGD("audio frame(\033[33m%s\033[0m)->\trate: %d\tchannels: %d\tfmt:%d\tPTS: %ld\n",
             tag,
             frame->sample_rate,
             frame->channels,
             frame->format,
             frame->pts);
    }
}

void logStream(AVStream *stream, const char *tag, int isVideo) {
    if (isVideo) {
        LOGD("video stream(\033[33m%s\033[0m)->\tindex: %d\ttimebase{%d, %d}\tduration: %ld\tframerate{%d, %d}\tstart: %ld\tend: %ld\n",
             tag,
             stream->index,
             stream->time_base.num,
             stream->time_base.den,
             stream->duration,
             stream->r_frame_rate.num,
             stream->r_frame_rate.den,
             stream->first_dts,
             stream->last_dts_for_order_check);
    } else {
        LOGD("audio stream(\033[33m%s\033[0m)->\tindex: %d\ttimebase{%d, %d}\tduration: %ld\tstart: %ld\tend: %ld\n",
             tag,
             stream->index,
             stream->time_base.num,
             stream->time_base.den,
             stream->duration,
             stream->first_dts,
             stream->last_dts_for_order_check);
    }
}
