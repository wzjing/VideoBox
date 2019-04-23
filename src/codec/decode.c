//
// Created by Infinity on 2019-04-20.
//

#include "decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include "../image/scale.h"
#include "../utils/snapshot.h"

#define INBUF_SIZE 4096

//static const char * out_filename;

//struct SwsContext *sws_ctx = NULL;
//uint8_t *dst_data[4];
//int dst_linesize[4];

//int byte_per_pixel = -1;

static void ppm_save(AVFrame *src, char *filename) {

//  if (byte_per_pixel == -1) {
//    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
//    byte_per_pixel = av_get_bits_per_pixel(desc);
//    printf("byte per pixel: %d\n", byte_per_pixel);
//  }
//
//  if (!sws_ctx) {
//    printf("initial SwsContext\n");
//    sws_ctx = sws_getContext(src->width, src->height, src->format,
//                             src->width, src->height, AV_PIX_FMT_RGB24,
//                             SWS_BILINEAR, NULL, NULL, NULL);
//  }
//
//  yuv2rgb(sws_ctx, src->data, src->linesize, src->width, src->height, dst_data, dst_linesize, src->width, src->height);
//
//  save_ppm(dst_data[0], dst_linesize[0], src->width, src->height, filename);
//
//  av_freep(&dst_data);
//  memset(dst_linesize, 0, sizeof(dst_linesize));
}

static void decode_packet(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, DECODE_CALLBACK callback) {
  int ret;

  ret = avcodec_send_packet(dec_ctx, pkt);
  if (ret < 0) {
    fprintf(stderr, "Error sending a packet for decoding\n");
    exit(1);
  }

  while (ret >= 0) {
    ret = avcodec_receive_frame(dec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    } else if (ret < 0) {
      fprintf(stderr, "Error during decoding\n");
      exit(1);
    }
    fflush(stdout);
    printf("Index: %d ", dec_ctx->frame_number);
    callback(frame);
  }
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

//  sws_freeContext(sws_ctx);
  return 0;
}
