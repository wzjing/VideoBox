//
// Created by Infinity on 2019-04-20.
//

#include "decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include "../utils/snapshot.h"

#define INBUF_SIZE 4096

int decode_packet(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, DECODE_CALLBACK callback) {
  int ret;

  ret = avcodec_send_packet(dec_ctx, pkt);
  if (ret < 0) {
    LOGE("Error sending a packet for decoding\n");
    exit(1);
  }

  while (ret >= 0) {
    ret = avcodec_receive_frame(dec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      LOGD("EOF\n");
      return 0;
    } else if (ret < 0) {
      LOGE("Error during decoding\n");
      exit(1);
    }
//    fflush(stdout);
    if (callback) callback(frame);
  }

  return pkt->size;
}

int decode_h264(const char *input_file, DECODE_CALLBACK callback) {

  const AVCodec *codec;
  AVCodecParserContext *parser;
  AVCodecContext *c = NULL;
  FILE *f;
  AVFrame *frame;
  uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
  uint8_t *data;
  size_t data_size;
  int ret;
  AVPacket *pkt;

  pkt = av_packet_alloc();
  if (!pkt) {
    exit(1);
  }

  memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!codec) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }

  parser = av_parser_init(codec->id);
  if (!parser) {
    fprintf(stderr, "parser not found\n");
    exit(1);
  }

  c = avcodec_alloc_context3(codec);
  if (!c) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  f = fopen(input_file, "rb");
  if (!f) {
    fprintf(stderr, "Could not open input file %s\n", input_file);
    exit(1);
  }

  frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Could not allocate frame\n");
    exit(1);
  }

  while (!feof(f)) {
    data_size = fread(inbuf, 1, INBUF_SIZE, f);
    if (!data_size) {
      break;
    }

    data = inbuf;
    while (data_size > 0) {
      ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data, data_size,
                             AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
      if (ret < 0) {
        fprintf(stderr, "Error while parsing\n");
        exit(1);
      }
      data += ret;
      data_size -= ret;
      if (pkt->size) {
        decode_packet(c, frame, pkt, callback);
      }
    }
  }

  // flush
  decode_packet(c, frame, NULL, NULL);

  fclose(f);

  av_parser_close(parser);
  avcodec_free_context(&c);
  av_frame_free(&frame);
  av_packet_free(&pkt);

  return 0;
}
