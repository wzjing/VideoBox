
#include "log.h"

void logPacket(AVPacket *packet, const char *tag) {
    char flags[3];
    flags[0] = packet->flags & AV_PKT_FLAG_KEY ? 'K' : '-';
    flags[1] = packet->flags & AV_PKT_FLAG_DISCARD ? 'D' : '-';
    char tag_str[20];
    snprintf(tag_str, 20, "[\033[33m%s\033[0m]", tag);
    LOGD("Packet%-16s->\tstream: %d\tPTS: %8ld\tDTS: %8ld\tDuration: %8ld\tflags:\033[34m%-8s\033[0m\tsize:%8d\tside_data: %s(%d)\n",
         tag_str,
         packet->stream_index,
         packet->pts,
         packet->dts,
         packet->duration,
         flags,
         packet->size,
         packet->side_data ? av_packet_side_data_name(packet->side_data->type) : "none",
         packet->side_data ? packet->side_data->size : 0);
}

void logFrame(AVFrame *frame, const char *tag, int isVideo) {
    char tag_str[20];
    snprintf(tag_str, 20, "[\033[33m%s\033[0m]", tag);
    if (isVideo) {
        const char *type;
        switch (frame->pict_type) {
            case AV_PICTURE_TYPE_I:
                type = "\033[36mI\033[0m";
                break;
            case AV_PICTURE_TYPE_B:
                type = "\033[34mB\033[0m";
                break;
            case AV_PICTURE_TYPE_P:
                type = "\033[35mP\033[0m";
                break;
            default:
                type = "\033[33m-\033[0m";
                break;
        }
        LOGD("VFrame%-16s->\ttype: video\tPTS: %8ld\tDTS: %8ld\tDuration: %8ld\tpict: %-16s\tfmt:%s\tsize: %dx%d\n",
             tag_str,
             frame->pts,
             frame->pkt_dts,
             frame->pkt_duration,
             type,
             av_get_pix_fmt_name((AVPixelFormat)frame->format),
             frame->width,
             frame->height);
    } else {
        LOGD("AFrame%-8s->type: audio\tPTS: %8ld\tDTS: %8ld\trate: %d\tchannels: %d\tfmt:%d\n",
             tag_str,
             frame->pts,
             frame->pkt_dts,
             frame->sample_rate,
             frame->channels,
             frame->format);
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
        LOGD("\n\033[34mAudio Context\033[0m(\033[33m%s\033[0m):\n"
             "\tcodec: %s\n"
             "\tsample:%s\n"
             "\tsample_rate: %d\n"
             "\tbitrate:  %ld\n"
             "\ttimebase: {%d, %d}\n"
             "\textradata_size: %d\n"
             "\tglobal_header: %s\n\n",
             tag,
             avcodec_get_name(context->codec_id),
             av_get_sample_fmt_name(context->sample_fmt),
             context->sample_rate,
             context->bit_rate,
             context->time_base.num,
             context->time_base.den,
             context->extradata_size,
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
