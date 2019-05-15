//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_CONCAT_H
#define VIDEOBOX_CONCAT_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
}

typedef struct Fragment {
    AVFormatContext *formatContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVCodecContext *videoCodecContext = nullptr;

} Framgment;

int concat(const char *input_filename, const char *input_filename1, const char * bgm_file, const char *output_filename);

#endif //VIDEOBOX_CONCAT_H
