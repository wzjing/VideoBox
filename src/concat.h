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

int concat(const char *output_filename, const char * bgm_file, char ** video_files, int nb_videos);

#endif //VIDEOBOX_CONCAT_H
