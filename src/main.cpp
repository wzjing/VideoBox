#include <cstdio>
#include <cstring>
#include <assert.h>

#include "codec/decode.h"
#include "utils/log.h"
#include "utils/io.h"
#include "demux_decode.hpp"
#include "mux_encode.hpp"
#include "audio_resample.hpp"
#include "audio_filter.hpp"
#include "video_encode_x264.hpp"
//#include "multi-mux.hpp"
#include "test/test_io.hpp"
#include "mux/mux.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
}

int main(int argc, char *argv[]) {

  auto check = [&argv](const char *command) -> bool {
    return strcmp(argv[1], command) == 0;
  };

  LOGD("configure: %s\n", avcodec_configuration());

  if (argc >= 1) {
    if (check("demux")) return demux_decode(argv[2], argv[3], argv[4]);
    else if (check("mux_exp")) return mux_encode(argv[2], argv[3], argv[4]);
    else if (check("mux")) {
      int ret = 0;
      Muxer *muxer = create_muxer(argv[2]);
      MediaConfig video_config;
      video_config.media_type = AVMEDIA_TYPE_VIDEO;
      video_config.codec_id = AV_CODEC_ID_H264;
      video_config.width = 1920;
      video_config.height = 1080;
      video_config.bit_rate = 4000000;
      video_config.format = AV_PIX_FMT_YUV420P;
      video_config.frame_rate = 30;
      video_config.gop_size = 12;
      Media *video = add_media(muxer, &video_config, nullptr);
//      MediaConfig audio_config;
//      audio_config.media_type = AVMEDIA_TYPE_AUDIO;
//      audio_config.codec_id = AV_CODEC_ID_AAC;
//      audio_config.format = AV_SAMPLE_FMT_FLTP;
//      audio_config.bit_rate = 64000;
//      audio_config.sample_rate = 44100;
//      audio_config.nb_samples = 1024;
//      audio_config.channel_layout = AV_CH_LAYOUT_STEREO;
//      Media *audio = add_media(muxer, &audio_config, nullptr);
      Media *audio = nullptr;

      AVFormatContext *fmt_ctx = muxer->fmt_ctx;

      av_dump_format(fmt_ctx, 0, muxer->output_filename, 1);

      FILE *v_file = fopen(argv[3], "rb");
      FILE *a_file = fopen(argv[4], "rb");
//      mux(muxer, [&video, &audio, &v_file, &a_file](AVFrame *frame, int type) -> int {
//        if (type == AVMEDIA_TYPE_VIDEO) {
//          return read_yuv(v_file, frame, frame->width, frame->height, video->codec_ctx->frame_number,
//                         (AVPixelFormat) frame->format);
//        } else if (type == AVMEDIA_TYPE_AUDIO) {
//          return read_pcm(a_file, frame, frame->nb_samples, frame->channels, audio->codec_ctx->frame_number,
//                         (AVSampleFormat) frame->format);
//        } else {
//          return 1;
//        }
//      }, nullptr);


      if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        LOGD("Opening file: %s\n", argv[2]);
        ret = avio_open(&fmt_ctx->pb, argv[2], AVIO_FLAG_WRITE);
        if (ret < 0) {
          LOGE("could not open %s (%s)\n", argv[2], av_err2str(ret));
          return -1;
        }
      }

      ret = avformat_write_header(muxer->fmt_ctx, nullptr);
      if (ret < 0) {
        LOGE("error occurred when opening output file %s", av_err2str(ret));
        return -1;
      }

      int encode_video = 1, encode_audio = 0;

      while (encode_video || encode_audio) {
        LOGD("---------------------------------start-----------------------------\n");
        LOGD("Video[%d]: %8ld Audio[%d]\n", encode_video, video->next_pts,
             encode_audio);
        AVCodecContext *codec_ctx = nullptr;
        AVStream *stream = nullptr;
        AVFrame *frame = nullptr;
        if (encode_video) {
          codec_ctx = video->codec_ctx;
          stream = video->stream;
          frame = video->frame;
          // GET Frame
          if (av_compare_ts(video->next_pts, codec_ctx->time_base,
                            STREAM_DURATION, (AVRational) {1, 1}) >= 0) {
            frame = nullptr;
          } else {
            if (av_frame_make_writable(frame) < 0) {
              LOGE("video frame not writable\n");
              exit(1);
            }

            // frame data
            read_yuv(v_file, frame, frame->width, frame->height, codec_ctx->frame_number,
                     (AVPixelFormat) frame->format);

            frame->pts = video->next_pts++;
          }


          AVPacket packet = {nullptr};
          av_init_packet(&packet);

          ret = encode_packet(codec_ctx, frame, &packet,
                              [&fmt_ctx, &codec_ctx, &stream](AVPacket *pkt) -> int {
                                if (pkt) {
                                  return write_frame(fmt_ctx, &codec_ctx->time_base, stream,
                                                     pkt);
                                } else {
                                  return 1;
                                }
                              });
          encode_video = ret != 1;
        } else if (encode_audio) {
          codec_ctx = audio->codec_ctx;
          stream = audio->stream;
          frame = audio->frame;
          // GET Frame
          if (av_compare_ts(audio->next_pts, codec_ctx->time_base,
                            STREAM_DURATION, (AVRational) {1, 1}) >= 0) {
            frame = nullptr;
          } else {
            if (av_frame_make_writable(frame) < 0) {
              LOGE("audio frame not writable\n");
              exit(1);
            }

            // frame data
            read_pcm(v_file, frame, frame->width, frame->height, codec_ctx->frame_number,
                     (AVSampleFormat) frame->format);

            frame->pts = audio->next_pts;
            audio->next_pts += audio->frame->nb_samples;
          }


          AVPacket packet = {nullptr};
          av_init_packet(&packet);

          ret = encode_packet(codec_ctx, frame, &packet,
                              [&fmt_ctx, &codec_ctx, &stream](AVPacket *pkt) -> int {
                                if (pkt) {
                                  return write_frame(fmt_ctx, &codec_ctx->time_base, stream,
                                                     pkt);
                                } else {
                                  return 1;
                                }
                              });
          encode_audio = ret != 1;
        } else {
          break;
        }
//        av_frame_unref(frame);
        LOGD("---------------------------------end-----------------------------\n");
      }

      fclose(v_file);
      fclose(a_file);

      av_write_trailer(fmt_ctx);

      if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
      }

      for (int i = 0; i < muxer->nb_media; ++i) {
        Media *media = muxer->media[i];
        avcodec_free_context(&media->codec_ctx);
        av_frame_free(&media->frame);
      }

      avformat_free_context(fmt_ctx);
      LOGE("finished\n");

      return 0;
    } else if (check("resample")) return resample(argv[2], argv[3], argv[4]);
    else if (check("audio_filter")) return audio_filter(argv[2], argv[3], argv[4]);
    else if (check("x264_encode")) return encode(argv[2], argv[3]);
//    else if (check("multi_mux")) return mux_multi(&argv[2], &argv[3], argv[4]);
    else if (check("io")) return test_io(argv[2], argv[3]);
    else if (check("av_dict")) {
      AVDictionary *opt;
      av_dict_set(&opt, "sample_rate", "44100", 0);
      av_dict_set(&opt, "sample_fmt", "FLTP", 0);
      assert(opt != nullptr);
      printf("count: %d\n", av_dict_count(opt));
      if (opt)
        av_dict_free(&opt);
      return 0;
    } else if (check("h264")) {
      /** h264 test */
      int ret = 0;
      AVFormatContext *fmt_ctx = nullptr;
      AVCodecContext *codec_ctx = nullptr;
      LOGD("filename: %s\n", argv[2]);
      ret = avformat_open_input(&fmt_ctx, argv[2], nullptr, nullptr);
      if (ret < 0) {
        LOGE("Could not open source file %s %s\n", argv[2], av_err2str(ret));
        return -1;
      }

      if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGE("Could not find stream information\n");
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

      AVStream *stream = fmt_ctx->streams[0];
      AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
      codec_ctx = avcodec_alloc_context3(codec);

      if (!codec_ctx) {
        LOGE("code context alloc error\n");
        return -1;
      }

      avcodec_parameters_to_context(codec_ctx, stream->codecpar);
      avcodec_open2(codec_ctx, codec, nullptr);

      AVPacket pkt;
      av_init_packet(&pkt);
      pkt.size = 0;
      pkt.data = nullptr;

      AVFrame *frame = av_frame_alloc();

      LOGD("stream timebase: %d/%d\n", stream->time_base.num, stream->time_base.den);
      LOGD("codec_ctx timebase: %d/%d\n", codec_ctx->time_base.num, codec_ctx->time_base.den);
      LOGD("codec_ctx size: %dx%d\n", codec_ctx->width, codec_ctx->height);
      LOGD("codec_ctx frame_rate: %d/%d\n", codec_ctx->framerate.num, codec_ctx->framerate.den);

//      while (av_read_frame(fmt_ctx, &pkt) == 0) {
//        AVPacket packet = pkt;
//        if (pkt.size) {
//          do {
//            ret = decode_packet(codec_ctx, frame, &packet, [codec_ctx](AVFrame *f) -> void {
//              const char *type;
//              switch (f->pict_type) {
//                case AV_PICTURE_TYPE_I:
//                  type = "I";
//                  break;
//                case AV_PICTURE_TYPE_B:
//                  type = "N";
//                  break;
//                case AV_PICTURE_TYPE_P:
//                  type = "P";
//                  break;
//                default:
//                  type = "--";
//                  break;
//              }
//              LOGD("frame #%d type:%s  dts: %ld pts: %ld\n", codec_ctx->frame_number, type, f->pkt_dts, f->pkt_dts);
//            });
//          } while (ret);
////          LOGD("timebase: %ld/%ld\n", pkt.pos, pkt.dts);
////          LOGD("stream: #%d -> index:%4ld\tpts:%-8s\tpts_time:%-8s\tdts:%-8s\tdts_time:%-8s\tduration:%-8s\tduration_time:%-8s\n",
////               stream->index,
////               stream->nb_frames,
////               av_ts2str(pkt.pts), av_ts2timestr(pkt.pts, time_base),
////               av_ts2str(pkt.dts), av_ts2timestr(pkt.dts, time_base),
////               av_ts2str(pkt.duration), av_ts2timestr(pkt.duration, time_base));
//        }
//        av_packet_unref(&packet);
//      }


      av_frame_free(&frame);
      av_packet_unref(&pkt);
      avformat_free_context(fmt_ctx);

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