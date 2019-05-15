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
#define LOGE(format, ...) fprintf(stderr,"\033[32m" format "\033[0m", ## __VA_ARGS__)

#endif //VIDEOBOX_LOG_H
