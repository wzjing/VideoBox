//
// Created by android1 on 2019/4/22.
//

#include "concat.h"
#include "../utils/log.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
}

inline void check(int ret){
  if (ret == 0) {
    LOGE("error: int %s line %d\n", __FILE__, __LINE__);
    exit(1);
  }
}

int concat(const char *input_filename, const char *output_filename) {
  int ret = 0;
  AVFormatContext *fmt_ctx = nullptr;

  ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, output_filename);
  check(ret);

  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    LOGD("Opening file: %s\n", output_filename);
    ret = avio_open(&fmt_ctx->pb, output_filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
      LOGE("could not open %s (%s)\n", output_filename, av_err2str(ret));
      return -1;
    }
  }




  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    avio_closep(&fmt_ctx->pb);
  }

  avformat_free_context(fmt_ctx);
  return 0;
}
