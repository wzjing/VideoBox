#include <cstdio>
#include <cstring>
#include <assert.h>

#include "utils/log.h"
#include "utils/io.h"
#include "demux_decode.h"
#include "mux_encode.h"
#include "resample.h"
#include "audio_filter.h"
#include "x264_encode.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

int main(int argc, char *argv[]) {

  if (argc >= 1) {
    if (strcmp(argv[1], "demux") == 0) {
      demux_decode(argv[2], argv[3], argv[4]);
      return 0;
    } else if (strcmp(argv[1], "mux") == 0) {
      mux_encode(argv[2], argv[3], argv[4]);
      return 0;
    } else if (strcmp(argv[1], "resample") == 0) {
      resample(argv[2], argv[3], argv[4]);
      return 0;
    } else if (strcmp(argv[1], "audio_filter") == 0) {
      audio_filter(argv[2], argv[3], argv[4]);
      return 0;
    } else if(strcmp(argv[1], "x264_encode") == 0) {
      return encode(argv[2], argv[3]);
    } else if (strcmp(argv[1], "io") == 0) {
      FILE *file = fopen(argv[2], "rb");
      FILE *result = fopen(argv[3], "wb");
      int sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
      int nb_samples = 1024;
      int channels = 2;

      AVFrame *frame = av_frame_alloc();

      frame->format = AV_SAMPLE_FMT_FLTP;
      frame->nb_samples = nb_samples;
      frame->channel_layout = AV_CH_LAYOUT_STEREO;
      frame->sample_rate = 44100;
      av_frame_get_buffer(frame, 0);
      av_frame_make_writable(frame);

      int ret = 0;
      if (!av_frame_is_writable(frame)) {
        LOGE("frame is not writable: %d\n", ret);
        exit(1);
      }
      for (int i = 0; i < 100; ++i) {
        LOGD("frame writable: %s\n", av_frame_is_writable(frame) ? "TRUE" : "FALSE");
        read_pcm(file, frame, nb_samples, channels, i, AV_SAMPLE_FMT_FLTP);
        for (int j = 0; j < frame->nb_samples; ++j) {
          for (int ch = 0; ch < frame->channels; ++ch) {
            fwrite(frame->data[ch] + j * sample_size, sample_size, 1, result);
          }
        }
      }

      av_frame_free(&frame);

      fclose(result);
      fclose(file);
      return 0;
    } else if (strcmp(argv[1], "av_dic") == 0) {
//      AVDictionary *opt;
//      av_dict_set(&opt, "sample_rate", "44100", 0);
//      av_dict_set(&opt, "sample_fmt", "FLTP", 0);
//      assert(opt != nullptr);
//      printf("count: %d\n", av_dict_count(opt));
//      if (opt)
//        av_dict_free(&opt);
      return 0;
    } else if (strcmp(argv[1], "h264") == 0) {
      int ret = 0;
      AVFormatContext* fmt_ctx;
      LOGD("filename: %s\n", argv[2]);
      ret = avformat_open_input(&fmt_ctx, argv[2], nullptr, nullptr);
      if (ret < 0) {
        LOGE("get_demuxer: Could not open source file %s %s\n", argv[2], av_err2str(ret));
        return -1;
      }

      if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGE("get_demuxer: Could not find stream information\n");
        return -1;
      }

      int have_video = 0;
      for (int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
          have_video = 1;
          break;
        }
      }
      if (!have_video) {
        LOGE("unable to read video from h264 file: %s\n", argv[2]);
        return -1;
      }
      LOGD("got h264\n");
      return 0;
    }
  }

  fprintf(stderr, "invalid command: ");
  for (int j = 0; j < argc; ++j) {
    printf("%s ", argv[j]);
  }
  printf("\n");
  return -1;
}