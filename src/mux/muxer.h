//
// Created by wzjing on 2019/5/14.
//

#ifndef VIDEOBOX_MUXER_H
#define VIDEOBOX_MUXER_H


#include <libavformat/avformat.h>

struct Track {
    unsigned int *streamId;
    AVStream *stream;
};

class Muxer {
private:
    AVFormatContext *fmtContext = nullptr;
    Track *videoTrack = nullptr;
    Track *audioTrack = nullptr;

public:
    void addTrack();

    void setOnAudioAvailableListener();

    void setOnVideoAvailableListener();

    void startMux();

    void stopMux();
};


#endif //VIDEOBOX_MUXER_H
