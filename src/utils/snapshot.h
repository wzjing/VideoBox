//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_SNAPSHOT_H
#define VIDEOBOX_SNAPSHOT_H

#include <libavutil/frame.h>

int save_pgm(uint8_t *buf, int wrap, int width, int height,
             const char *filename);

int save_ppm(uint8_t *buf, int wrap, int width, int height,
             const char *filename);

int save_yuv(uint8_t **buf, const int *wrap, int width,
        int height, const char *filename);

int save_av_frame(AVFrame * frame, char * filename);

#endif //VIDEOBOX_SNAPSHOT_H
