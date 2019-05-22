//
// Created by android1 on 2019/5/22.
//

#ifndef VIDEOBOX_CONCAT_ADD_TITLE_H
#define VIDEOBOX_CONCAT_ADD_TITLE_H

struct Video {
    AVFormatContext *formatContext = nullptr;
    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVCodecContext *videoCodecContext = nullptr;

};

int concat_add_title(const char * output_filename, char** input_filenames, size_t nb_inputs);

#endif //VIDEOBOX_CONCAT_ADD_TITLE_H
