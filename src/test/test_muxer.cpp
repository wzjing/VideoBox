#include "test_muxer.h"

int test_muxer(const char * output_filename, const char * input_video, const char * input_audio) {
    Muxer *muxer = create_muxer(output_filename);
    AVDictionary *video_opt = nullptr;
    av_dict_set(&video_opt, "preset", "fast", 0);
    MediaConfig video_config;
    video_config.media_type = AVMEDIA_TYPE_VIDEO;
    video_config.codec_id = AV_CODEC_ID_H264;
    video_config.width = 1920;
    video_config.height = 1080;
    video_config.bit_rate = 7200000;
    video_config.format = AV_PIX_FMT_YUV420P;
    video_config.frame_rate = 30;
    video_config.gop_size = 250;
    Media *video = add_media(muxer, &video_config, video_opt);
    av_dict_free(&video_opt);
    MediaConfig audio_config;
    audio_config.media_type = AVMEDIA_TYPE_AUDIO;
    audio_config.codec_id = AV_CODEC_ID_AAC;
    audio_config.format = AV_SAMPLE_FMT_FLTP;
    audio_config.bit_rate = 64000;
    audio_config.sample_rate = 44100;
    audio_config.nb_samples = 1024;
    audio_config.channel_layout = AV_CH_LAYOUT_STEREO;
    Media *audio = add_media(muxer, &audio_config, nullptr);

    FILE *v_file = fopen(input_video, "rb");
    FILE *a_file = fopen(input_audio, "rb");
    mux(muxer, [&video, &audio, &v_file, &a_file](AVFrame *frame, int type) -> int {
        if (type == AVMEDIA_TYPE_VIDEO) {
            return read_yuv(v_file, frame, frame->width, frame->height, video->codec_ctx->frame_number,
                            (AVPixelFormat) frame->format);
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            return read_pcm(a_file, frame, frame->nb_samples, frame->channels, audio->codec_ctx->frame_number,
                            (AVSampleFormat) frame->format);
        } else {
            return 1;
        }
    }, nullptr);


    close_muxer(muxer);
    return 0;
}