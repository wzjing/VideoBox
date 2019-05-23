//
// Created by android1 on 2019/5/14.
//

#include "Track.h"

Track::Track(AVMediaType type, AVCodecID codecId, AVStream *stream, unsigned int streamIdx) :
        mediaType(type),
        codecId(codecId),
        pStream(stream),
        streamIdx(streamIdx) {

}

int Track::getType() {
    return this->mediaType;
}

AVCodecContext *Track::getCodecContext() {
    return this->pCodecContext;
}

AVStream *Track::getStream() {
    return this->pStream;
}

unsigned int Track::getStreamIndex() {
    return this->streamIdx;
}
