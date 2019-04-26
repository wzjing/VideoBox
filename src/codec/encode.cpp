//
// Created by Infinity on 2019-04-20.
//

#include "encode.h"


void encode_packet(AVCodecContext * enc_ctx, AVFrame * frame, AVPacket * packet, ENCODE_CALLBACK callback) {
  if (!frame) return;
  int ret;
  ret = avcodec_send_frame(enc_ctx, frame);
  if (ret<0) {
    LOGE("encode_packet: error sending frame\n");
    exit(1);
  }

  while(ret>=0) {
    ret = avcodec_receive_packet(enc_ctx, packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    } else if (ret < 0) {
      LOGE("encode_packet: error receive packet\n");
      exit(1);
    }

    (*callback)(packet);
    av_packet_unref(packet);
  }
}