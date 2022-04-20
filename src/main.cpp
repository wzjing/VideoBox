#include <cstdio>
#include <cstring>

#include "utils/log.h"
#include "test/test_demuxer.h"
#include "test/mux_encode.h"
#include "test/test_muxer.h"
#include "test/x264.h"
#include "test/get_info.h"
#include "mux/mux.h"
#include "functions/concat_bgm.h"
#include "functions/mux_title.h"
#include "functions/mix_bgm.h"
#include "functions/concat_add_title.h"
#include "functions/rtmp.h"
#include "functions/filters.h"
#include <regex>

int main(int argc, char *argv[]) {

    auto check = [&argv](const char *command) -> bool {
        return strcmp(argv[1], command) == 0;
    };

    if (argc >= 1) {
        if (check("info")) return get_info(argv[2]);
        else if (check("mux_exp")) return mux_encode(argv[2], argv[3], argv[4]);
        else if (check("muxer")) return test_muxer(argv[2], argv[3], argv[4]);
        else if (check("mux")) return test_mux(argv[2], argv[3], argv[4], 4);
        else if (check("remux")) return test_remux(argv[2], argv[3]);
        else if (check("demuxer")) return test_demuxer(argv[2], argv[3], argv[4]);
        else if (check("concat")) return concat(argv[2], argv[3], &argv[4], 4);
        else if (check("concat_add_title")) return concat_add_title(argv[2], &argv[3], 2);
        else if (check("mux_title")) return mux_title(argv[2], argv[3]);
        else if (check("bgm")) return mix_bgm(argv[2], argv[3], argv[4], 1.8);
        else if (check("x264_encode")) return x264_encode(argv[2], argv[3]);
        else if (check("rtmp")) return play_rtmp(argv[2]);
        else if (check("filter_blur")) return filter_blur(argv[2], argv[3]);
        else if (check("filter_mix")) return filter_mix(argv[2], argv[3], argv[4]);
        else if (check("test")) {
            const char * output_filename = "/mnt/0/emulated/Download.d/video.mp4";

            std::string cache_filename(output_filename);

            cache_filename = std::regex_replace(cache_filename, std::regex(".[0-9a-zA-Z]+$"), "_cache.ts");

            LOGD("Replaced: %s\n", cache_filename.c_str());
            LOGD("Origin: %s\n", output_filename);

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