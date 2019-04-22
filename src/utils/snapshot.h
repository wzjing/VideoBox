//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_SNAPSHOT_H
#define VIDEOBOX_SNAPSHOT_H

#include <libavutil/frame.h>

int save_pgm(unsigned char *buf, int wrap, int width, int height,
             char *filename);

int save_ppm(unsigned char *buf, int wrap, int width, int height,
             char *filename);

int save_yuv(unsigned char **bufs, const int *wraps, int width, 
        int height, char *filename);

int save_av_frame(AVFrame * frame, char * filename);

#endif //VIDEOBOX_SNAPSHOT_H
