//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_LOG_H
#define VIDEOBOX_LOG_H

#include <cstdio>

#define LOG(format, ...) printf(format, ## __VA_ARGS__)
#ifdef DEBUG
#define LOGD(format, ...) printf(format, ## __VA_ARGS__)
#else
#define LOGD(format, ...)
#endif
#define LOGI(format, ...) printf("\033[34m" format "\033[0m", ## __VA_ARGS__)
#define LOGW(format, ...) printf("\033[33m" format "\033[0m", ## __VA_ARGS__)
#define LOGE(format, ...) fprintf(stderr,"\033[31m" format "\033[0m", ## __VA_ARGS__)

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
}

void logMetadata(AVDictionary* metadata, const char * tag);

void logContext(AVCodecContext *context, const char *tag, int isVideo);

void logStream(AVStream* stream, const char * tag, int isVideo);

void logPacket(AVPacket *packet, const char *tag);

void logFrame(AVFrame *frame, const char *tag, int isVideo);

#endif //VIDEOBOX_LOG_H
