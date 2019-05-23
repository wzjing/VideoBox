#ifndef TEST_MUXER_H
#define TEST_MUXER_H

#include "../mux/mux.h"
#include "../utils/io.h"

int test_muxer(const char * output_filename, const char * input_video, const char * input_audio);

int test_mux(const char * output_filename, const char * input_video, const char * input_audio, int64_t duration);

int test_remux(const char * in_filename, const char * out_filename);

#endif // TEST_MUXER_H