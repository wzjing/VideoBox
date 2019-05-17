
#include "log.h"

void logPacket(AVPacket *packet, const char *tag) {
    LOGD("packet(\033[32m%s\033[0m)->\tstream: %d\tkey:%s\tsize:%-8d\tPTS: %-8ld\tDTS: %-8ld\tCorrupted: %s\tDisposable: %s\tDiscard: %s\n",
         tag,
         packet->stream_index,
         packet->flags & AV_PKT_FLAG_KEY ? "Yes" : "No",
         packet->size,
         packet->pts,
         packet->dts,
         packet->flags & AV_PKT_FLAG_CORRUPT? "Yes":"No",
         packet->flags & AV_PKT_FLAG_DISPOSABLE? "Yes":"No",
         packet->flags & AV_PKT_FLAG_DISCARD? "Yes":"No");
}

void logFrame(AVFrame *frame, const char *tag, int isVideo) {
    if (isVideo) {
        const char *type;
        switch (frame->pict_type) {
            case AV_PICTURE_TYPE_I:
                type = "\033[34mI\033[0m";
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
        LOGD("VFrame(\033[33m%s\033[0m)->\ttype:%s\tfmt:%d\tsize: %dx%d\tPTS: %8ld\n",
             tag,
             type,
             frame->format,
             frame->width,
             frame->height,
             frame->pts);
    } else {
        LOGD("AFrame(\033[33m%s\033[0m)->\trate: %d\tchannels: %d\tfmt:%d\tPTS: %ld\n",
             tag,
             frame->sample_rate,
             frame->channels,
             frame->format,
             frame->pts);
    }
}

void logStream(AVStream *stream, const char *tag, int isVideo) {
    if (isVideo) {
        LOGD("\n\033[34mVideo Stream\033[0m(\033[33m%s\033[0m):\n"
             "\tindex: %d\n"
             "\ttimebase:     {%d, %d}\n"
             "\tframerate:    {%d, %d}\n"
             "\tdisplay ratio:%d:%d\n"
             "\tduration: %ld\n"
             "\tstart:    %ld\n\n",
             tag,
             stream->index,
             stream->time_base.num,
             stream->time_base.den,
             stream->r_frame_rate.num,
             stream->r_frame_rate.den,
             stream->display_aspect_ratio.num,
             stream->display_aspect_ratio.den,
             stream->duration,
             stream->first_dts == AV_NOPTS_VALUE ? -1 : stream->first_dts);
    } else {
        LOGD("\n\033[34mAudio Stream\033[0m(\033[33m%s\033[0m)-:\n"
             "\tindex: %d\n"
             "\ttimebase: {%d, %d}\n"
             "\tduration: %ld\n"
             "\tstart:    %ld\n\n",
             tag,
             stream->index,
             stream->time_base.num,
             stream->time_base.den,
             stream->duration,
             stream->first_dts == AV_NOPTS_VALUE ? -1 : stream->first_dts);
    }
}

void logContext(AVCodecContext *context, const char *tag, int isVideo) {
    if (isVideo) {
        LOGD("\n\033[34mVideo Context\033[0m(\033[33m%s\033[0m):\n"
             "\tcodec: %s\n"
             "\tpix:   %s\n"
             "\tsize:  %dx%d\n"
             "\tframerate: {%d, %d}\n"
             "\ttimebase:  {%d, %d}\n"
             "\tgroup: %d\n"
             "\thas_b: %s\n"
             "\tmax_b: %d\n"
             "\tq_max: %d\n"
             "\tq_min: %d\n"
             "\tprofile: %d\n"
             "\textradata_size: %d\n"
             "\tglobal_header: %s\n\n",
             tag,
             avcodec_get_name(context->codec_id),
             av_get_pix_fmt_name(context->pix_fmt),
             context->width,
             context->height,
             context->framerate.num,
             context->framerate.den,
             context->time_base.num,
             context->time_base.den,
             context->gop_size,
             context->has_b_frames ? "YES" : "NO",
             context->max_b_frames,
             context->qmax,
             context->qmin,
             context->profile,
             context->extradata_size,
             context->flags & AV_CODEC_FLAG_GLOBAL_HEADER ? "YES" : "NO"
        );
    } else {
        LOGD("\n\033[34mAudio Context\033[0m(\033[33m%s\033[0m):"
             "\tcodec: %s\n"
             "\tsample:%s\n"
             "\tsample_rate: %d\n"
             "\tbitrate:  %ld\n"
             "\ttimebase: {%d, %d}\n"
             "\tglobal_header: %s\n\n",
             tag,
             avcodec_get_name(context->codec_id),
             av_get_sample_fmt_name(context->sample_fmt),
             context->sample_rate,
             context->bit_rate,
             context->time_base.num,
             context->time_base.den,
             context->flags & AV_CODEC_FLAG_GLOBAL_HEADER ? "YES" : "NO"
        );
    }
}

void logMetadata(AVDictionary *metadata, const char *tag) {
    LOGD("\nmetadata(\033[33m%s\033[0m):\n", tag);
    AVDictionaryEntry *item = nullptr;
    while ((item = av_dict_get(metadata, "", item, AV_DICT_IGNORE_SUFFIX)))
        printf("\t%s:\t%s\n", item->key, item->value);
    LOGD("\n");
}
