//
// Created by android1 on 2019/5/14.
//

#ifndef VIDEOBOX_TRACK_H
#define VIDEOBOX_TRACK_H

#include "mux_common.h"

class Track {
protected:
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    AVCodecID codecId = AV_CODEC_ID_NONE;
    const AVCodec *codec;
    AVCodecContext *pCodecContext = nullptr;
    AVStream *pStream = nullptr;
    unsigned int streamIdx = 0;
public:

    Track(AVMediaType type, AVCodecID codecId, AVStream *stream, unsigned int streamIdx);

    int getType();

    AVCodecContext *getCodecContext();

    AVStream *getStream();

    unsigned int getStreamIndex();
};


#endif //VIDEOBOX_TRACK_H
