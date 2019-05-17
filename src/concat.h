//
// Created by android1 on 2019/4/22.
//

#ifndef VIDEOBOX_CONCAT_H
#define VIDEOBOX_CONCAT_H

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

int concat_with_bgm(const char *output_filename, const char *bgm_file, char **video_files, int nb_videos);

int concat(const char *output_filename, char **video_files, int nb_videos);

inline int error(const char *message) {
    LOGE("error: file %s line %d\n\t\033[31m%s\033[0m\n", __FILE__, __LINE__, message);
    return -1;
}

#endif //VIDEOBOX_CONCAT_H
