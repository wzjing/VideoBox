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

#define INBUF_SIZE 4096

struct SwsContext *sws_ctx = NULL;
uint8_t *dst_data[4];
int dst_linesize[4];

int byte_per_pixel = -1;

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename) {
  FILE *f;
  int i;

  f = fopen(filename, "w");
  fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
  for (i = 0; i < ysize; i++)
    fwrite(buf + i * wrap, 1, xsize, f);
  fclose(f);
}

static void ppm_save(AVFrame *src, char *filename) {

  if (byte_per_pixel == -1) {
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
    byte_per_pixel = av_get_bits_per_pixel(desc);
    printf("byte per pixel: %d\n", byte_per_pixel);
  }

  if (!sws_ctx) {
    printf("initial SwsContext\n");
    sws_ctx = sws_getContext(src->width, src->height, src->format,
                             src->width, src->height, AV_PIX_FMT_RGB24,
                             SWS_BILINEAR, NULL, NULL, NULL);
  }

  yuv2rgb(sws_ctx, src->data, src->linesize, src->width, src->height, dst_data, dst_linesize, src->width, src->height);

  FILE *f;

  f = fopen(filename, "w");
  fprintf(f, "P6\n%d %d\n%d\n", src->width, src->height, 255);
  for (int i = 0; i < src->height; i++) {
    fwrite(dst_data[0] + i * dst_linesize[0], byte_per_pixel / 8, src->width, f);
  }
  fclose(f);

  printf("linesize:(%dx%d) %d %d %d %d\n",
         src->width,
         src->height,
         dst_linesize[0],
         dst_linesize[1],
         dst_linesize[2],
         dst_linesize[3]);

  av_freep(&dst_data);
  memset(dst_linesize, 0, sizeof(dst_linesize));
}

static void yuv_save(AVFrame *frame, const char *filename) {
  FILE *f;
  f = fopen(filename, "w");

  for (int i = 0; i < frame->height; i++) {
    fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->width, f);
  }

  for (int i = 0; i < frame->height / 2; i++) {
    fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->width / 2, f);
  }

  for (int i = 0; i < frame->height; i++) {
    fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->width / 2, f);
  }

  fclose(f);
}

static void decode_packet(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, const char *filename) {
  char buf[1024];
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

    printf("saving frame %3d (%s: %dx%d)\n",
           dec_ctx->frame_number,
           av_get_sample_fmt_name(frame->format),
           frame->width,
           frame->height);
    fflush(stdout);

    snprintf(buf, sizeof(buf), "%s-%d%s", filename, dec_ctx->frame_number, ".ppm");
//    pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
    ppm_save(frame, buf);
//    yuv_save(frame, buf);
  }
}

int decode_h264(const char *input_file, const char *output_file) {
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
        decode_packet(c, frame, pkt, output_file);
      }
    }
  }

  // flush
  decode_packet(c, frame, NULL, output_file);

  fclose(f);

  av_parser_close(parser);
  avcodec_free_context(&c);
  av_frame_free(&frame);
  av_packet_free(&pkt);

  sws_freeContext(sws_ctx);
  return 0;
}
