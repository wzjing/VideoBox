//
// Created by android1 on 2019/4/26.
//

#include "io.h"


int read_yuv(FILE *file, AVFrame *frame, int width, int height, int index,
             enum AVPixelFormat pix_fmt) {
  int ret = 0;
  if (!av_frame_is_writable(frame)) {
    LOGE("read_yuv: AVFrame is not writable\n");
    return -1;
  }

  frame->width = width;
  frame->height = height;
  frame->format = pix_fmt;

  switch (pix_fmt) {
    case AV_PIX_FMT_YUV420P:
      ret = fseeko(file, width * height * 3 / 2 * index, SEEK_SET);
      if (ret != 0) {
        LOGD("seek file error: %d\n", ret);
        return -1;
      }
      frame->linesize[0] = width;
      frame->linesize[1] = width / 2;
      frame->linesize[2] = width / 2;
      frame->data[0] = (uint8_t *) (malloc(sizeof(uint8_t) * width * height));
      frame->data[1] = (uint8_t *) (malloc(sizeof(uint8_t) * width * height / 4));
      frame->data[2] = (uint8_t *) (malloc(sizeof(uint8_t) * width * height / 4));
      fread(frame->data[0], 1, width * height, file);
      fread(frame->data[1], 1, width * height / 4, file);
      fread(frame->data[2], 1, width * height / 4, file);
      return 0;
    case AV_PIX_FMT_RGB24:
      fseeko(file, width * height * 3 * index, SEEK_SET);
      frame->linesize[0] = width * 3;
      frame->data[0] = (uint8_t *) (malloc(sizeof(uint8_t) * width * height * 3));
      fread(frame->data[0], 1, width * height * 3, file);
      return 0;
    default:
      LOGE("unknown pixel format\n");
      return 0;
  }
}

int read_pcm(FILE *file, AVFrame *frame, int nb_samples, int channels, int index,
             enum AVSampleFormat sample_fmt) {
  int ret = 0;
  if (!av_frame_is_writable(frame)) {
    LOGE("read_pcm: AVFrame is not writable\n");
    return 0;
  }
//  frame->nb_samples = nb_samples;
//  frame->channels = channels;
//  if (channels == 1) {
//    frame->channel_layout = AV_CH_LAYOUT_MONO;
//  } else if (channels == 2) {
//    frame->channel_layout = AV_CH_LAYOUT_STEREO;
//  }
//  frame->format = sample_fmt;

  int sample_size = av_get_bytes_per_sample(sample_fmt);
  switch (sample_fmt) {
    case AV_SAMPLE_FMT_FLTP:
      ret = fseeko(file, channels * sample_size * nb_samples * index, SEEK_SET);
      if (ret != 0) {
        LOGD("seek file error: %d\n", ret);
        return -1;
      }
      for (int i = 0; i < nb_samples; ++i) {
        uint8_t buf[sample_size * channels];
        fread(buf, sample_size, channels, file);
        for (int ch = 0; ch < channels; ++ch) {
          memcpy(frame->data[ch] + i * sample_size, buf + ch * sample_size, sample_size);
        }
      }
      LOGD("got frame\n");
      break;
    case AV_SAMPLE_FMT_S16P:
      LOGD("TBD..\n");
      break;
    default:
      LOGE("unknown sample format\n");
      break;
  }
  return 0;
}

void
read_pcm_to_raw(FILE *file, uint8_t **data, int nb_samples, int channels, int index, enum AVSampleFormat sample_fmt) {
  int sample_size = av_get_bytes_per_sample(sample_fmt);
  switch (sample_fmt) {
    case AV_SAMPLE_FMT_FLTP:
      fseeko64(file, channels * sample_size * nb_samples * index, SEEK_SET);
      for (int i = 0; i < nb_samples; ++i) {
        uint8_t buf[sample_size * channels];
        fread(buf, sample_size, channels, file);
        for (int ch = 0; ch < channels; ++ch) {
          memcpy(data[ch] + i * sample_size, buf + ch * sample_size, sample_size);
        }
      }
      break;
    case AV_SAMPLE_FMT_U8:
      LOGD("TBD..\n");
      break;
    default:
      LOGE("unknown sample format\n");
      break;
  }
}
