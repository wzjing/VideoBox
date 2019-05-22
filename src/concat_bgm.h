//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_CONCAT_BGM_H
#define VIDEOBOX_CONCAT_BGM_H

#include "./utils/log.h"
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

inline int error(const char *message) {
    LOGE("error: file %s line %d\n\t\033[31m%s\033[0m\n", __FILE__, __LINE__, message);
    return -1;
}

int concat(const char *output_filename, char *bgm_file, char **video_files, int nb_videos);

#endif //VIDEOBOX_CONCAT_BGM_H
