//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_LOG_H
#define VIDEOBOX_LOG_H

#include <stdio.h>

#define LOG(format, ...) printf(format, ## __VARARGS__)
#ifdef DEBUG
#define LOGD(format, ...) printf(format, ## __VARARGS__)
#else
#define LOGD(format, ...)
#endif
#define LOGE(format, ...) snprintf(stderr, format, ## __VAR_ARGS__)

#endif //VIDEOBOX_LOG_H
