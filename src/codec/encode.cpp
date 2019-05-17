//
// Created by Infinity on 2019-04-20.
//

#include "encode.h"

int encode_packet(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *packet, ENCODE_CALLBACK callback) {
    int ret;
//  if (frame == nullptr) goto receive;
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret == AVERROR_EOF) {
        LOGD("ENCODE-INPUT: eof\n");
    } else if (ret < 0) {
        LOGE("encode_packet: error sending frame\n");
        return -1;
    }

    receive:
    ret = avcodec_receive_packet(enc_ctx, packet);
    switch (ret) {
        case 0:
//      LOGD("ENCODE-OUT: got packet\n");
            callback(packet);
            goto receive;
        case AVERROR(EAGAIN):
//      LOGD("ENCODE-OUT: try again\n");
            return 0;
        case AVERROR_EOF:
//      LOGD("\033[33mENCODE-OUT: eof\033[0m\n");
            return 1;
        default:
            LOGE("encode_packet: error sending frame: %s\n", av_err2str(ret));
            return -1;
    }
}

int encode_packet_bsf(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *packet, AVBSFContext *bsfContext,
                      const std::function<int(AVPacket *)> &callback) {
    int ret;
//  if (frame == nullptr) goto receive;
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret == AVERROR_EOF) {
        LOGD("ENCODE-INPUT: eof\n");
    } else if (ret < 0) {
        LOGE("encode_packet: error sending frame\n");
        return -1;
    }

    receive:
    ret = avcodec_receive_packet(enc_ctx, packet);
    switch (ret) {
        case 0:
//      LOGD("ENCODE-OUT: got packet\n");
            ret = av_bsf_send_packet(bsfContext, packet);
            if (ret < 0) {
                LOGE("unable to send packet to filter:%s\n", av_err2str(ret));
                callback(packet);
            } else {
                do {
                    ret = av_bsf_receive_packet(bsfContext, packet);
                    if (ret == 0) {
                        callback(packet);
                    } else {
                        LOGW("unable to receive packet from filter:%s\n", av_err2str(ret));
                    }
                } while (ret == 0);
            }
            goto receive;
        case AVERROR(EAGAIN):
//      LOGD("ENCODE-OUT: try again\n");
            return 0;
        case AVERROR_EOF:
//      LOGD("\033[33mENCODE-OUT: eof\033[0m\n");
            return 1;
        default:
            LOGE("encode_packet: error sending frame: %s\n", av_err2str(ret));
            return -1;
    }
}
