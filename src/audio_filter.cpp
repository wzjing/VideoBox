//
// Created by android1 on 2019/4/29.
//

#include "audio_filter.h"
#include "utils/log.h"
#include "utils/io.h"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
}

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define NB_SAMPLES 1024


int create_filter(const char *filter_name, const char *instance_name, const AVFilter *&filter,
                  AVFilterContext *&filter_ctx, AVFilterGraph *&graph,
                  AVDictionary *opt) {
  filter = avfilter_get_by_name(filter_name);
  if (!filter) {
    LOGE("unable to find filter: %s\n", filter_name);
    return -1;
  }
  LOGD("got filter: %s\n", filter_name);

  filter_ctx = avfilter_graph_alloc_filter(graph, filter, instance_name);
  if (!filter_ctx) {
    LOGE("unable to create filter context\n");
    return -1;
  }
  LOGD("allocated filter context: %s\n", filter_name);

  int ret = avfilter_init_dict(filter_ctx, &opt);
  if (ret < 0) {
    LOGE("unable to init filter with options: %s\n", av_err2str(ret));
    return -1;
  }

  LOGD("filter create finished: %s\n", filter_name);
  return 0;
}

void get_frame(FILE *file, AVFrame *frame, int index) {
  frame->format = AV_SAMPLE_FMT_FLTP;
  frame->sample_rate = SAMPLE_RATE;
  frame->nb_samples = NB_SAMPLES;
  frame->channels = 2;
  frame->channel_layout = AV_CH_LAYOUT_STEREO;
  av_frame_get_buffer(frame, 0);
  av_frame_make_writable(frame);
  read_pcm(file, frame, NB_SAMPLES, CHANNELS, index, AV_SAMPLE_FMT_FLTP);
}


void audio_filter(const char *source_a, const char *source_b, const char *result) {
  AVFilterGraph *filter_graph;
  AVFilterContext *buffer_a_ctx, *buffer_b_ctx, *volume_a_ctx, *volume_b_ctx, *mix_ctx, *format_ctx, *sink_ctx;
  const AVFilter *buffer_a, *buffer_b, *volume_a, *volume_b, *mix, *format, *sink;
  int ret;
  int sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);

  char ch_layout[64];
  char sample_fmt[10];

  filter_graph = avfilter_graph_alloc();
  if (!filter_graph) {
    LOGE("unable to create filter graph\n");
    exit(1);
  }
  av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, AV_CH_LAYOUT_STEREO);
  sprintf(sample_fmt, "%d", AV_SAMPLE_FMT_FLTP);

  // source a filter
  AVDictionary *opt1 = nullptr;
  av_dict_set(&opt1, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt1, "sample_fmt", sample_fmt, AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt1, "time_base", "1/44100", AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt1, "sample_rate", AV_STRINGIFY(SAMPLE_RATE), AV_OPT_SEARCH_CHILDREN);
  create_filter("abuffer", "src0", buffer_a, buffer_a_ctx, filter_graph, opt1);

  // source b filter
  AVDictionary *opt2 = nullptr;
  av_dict_set(&opt2, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt2, "sample_fmt", sample_fmt, AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt2, "time_base", "1/44100", AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt2, "sample_rate", AV_STRINGIFY(SAMPLE_RATE), AV_OPT_SEARCH_CHILDREN);
  create_filter("abuffer", "src1", buffer_b, buffer_b_ctx, filter_graph, opt2);

  // volume a filter
  AVDictionary *opt3 = nullptr;
  av_dict_set(&opt3, "volume", "1.4", AV_OPT_SEARCH_CHILDREN);
  create_filter("volume", "volume0", volume_a, volume_a_ctx, filter_graph, opt3);

  // volume b filter
  AVDictionary *opt4 = nullptr;
  av_dict_set(&opt4, "volume", "0.6", AV_OPT_SEARCH_CHILDREN);
  create_filter("volume", "volume1", volume_b, volume_b_ctx, filter_graph, opt4);

  // mix filter
  create_filter("amix", "amix", mix, mix_ctx, filter_graph, nullptr);

  // format filter
  AVDictionary *opt5 = nullptr;
  av_dict_set(&opt5, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt5, "sample_fmt", sample_fmt, AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt5, "time_base", "1/44100", AV_OPT_SEARCH_CHILDREN);
  av_dict_set(&opt5, "sample_rate", AV_STRINGIFY(SAMPLE_RATE), AV_OPT_SEARCH_CHILDREN);
  create_filter("aformat", "aformat", format, format_ctx, filter_graph, opt5);

  // sink filter
  create_filter("abuffersink", "sink", sink, sink_ctx, filter_graph, nullptr);

  avfilter_link(buffer_a_ctx, 0, volume_a_ctx, 0);
  avfilter_link(buffer_b_ctx, 0, volume_b_ctx, 0);
  avfilter_link(volume_a_ctx, 0, mix_ctx, 0);
  avfilter_link(volume_b_ctx, 0, mix_ctx, 1);
  avfilter_link(mix_ctx, 0, format_ctx, 0);
  avfilter_link(format_ctx, 0, sink_ctx, 0);

  ret = avfilter_graph_config(filter_graph, nullptr);
  if (ret < 0) {
    LOGE("unable to configure filter graph: %s\n", av_err2str(ret));
    exit(1);
  }
  printf("Graph: %s\n", avfilter_graph_dump(filter_graph, nullptr));

  FILE *source_a_file = fopen(source_a, "rb");
  FILE *source_b_file = fopen(source_b, "rb");
  FILE *result_file = fopen(result, "wb");

  AVFrame *frame = av_frame_alloc();
  LOGD("start process\n");
  for (int i = 0; i < 180; ++i) {

    get_frame(source_a_file, frame, i);

    ret = av_buffersrc_add_frame(buffer_a_ctx, frame);
    if (ret < 0) {
      av_frame_unref(frame);
      LOGE("unable to add frame to buffer a context");
      goto end;
    }

    get_frame(source_b_file, frame, i);
    ret = av_buffersrc_add_frame(buffer_b_ctx, frame);
    if (ret < 0) {
      av_frame_unref(frame);
      LOGE("unable to add frame to buffer b context");
      goto end;
    }
    LOGD("start receive\n");
    av_frame_unref(frame);
    while ((ret = av_buffersink_get_frame(sink_ctx, frame)) >= 0) {
      LOGD("nb_sample: %d channels: %d\n", frame->nb_samples, frame->channels);
      for (int j = 0; j < frame->nb_samples; ++j) {
        for (int k = 0; k < frame->channels; ++k) {
          fwrite(frame->data[k] + j * sample_size, sample_size, 1, result_file);
        }
      }
      av_frame_unref(frame);
    }

    if (ret == AVERROR(EAGAIN)) {
      continue;
    } else if (ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      LOGE("unable to filter frame: %s\n", av_err2str(ret));
      goto end;
    }
  }

  avfilter_graph_free(&filter_graph);
  av_frame_free(&frame);
  end:
  fclose(source_a_file);
  fclose(source_b_file);
  fclose(result_file);
}