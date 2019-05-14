//
// Created by android1 on 2019/5/14.
//

#ifndef VIDEOBOX_TRACK_H
#define VIDEOBOX_TRACK_H

#include "mux_comman.h"

class Track {
protected:
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    AVCodecID codecId = AV_CODEC_ID_NONE;
    AVCodec *codec = nullptr;
    AVCodecContext *pCodecContext = nullptr;
    AVStream *pStream = nullptr;
    uint streamIdx = 0;
public:

    Track(AVMediaType type, AVCodecID codecId, AVStream *stream, uint streamIdx);

    int getType();

    AVCodecContext *getCodecContext();

    AVStream *getStream();

    uint getStreamIndex();
};


#endif //VIDEOBOX_TRACK_H
