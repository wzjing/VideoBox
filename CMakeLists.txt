cmake_minimum_required(VERSION 3.10)
project(VideoBox)

#set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include(FindPkgConfig)

add_definitions(-DDEBUG -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS)

if (${PKG_CONFIG_FOUND})
    message(STATUS "found pkg-config ${PKG_CONFIG_VERSION_STRING} in ${PKG_CONFIG_EXECUTABLE}")
else ()
    message(FATAL_ERROR "no pkg-config found, please install pkg-config and add libraries")
endif ()

pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libavformat libavfilter libavdevice libswscale libswresample libpostproc x264)

if (FFMPEG_FOUND)
    foreach (cflag ${FFMPEG_CFLAGS})
        message(STATUS "cflags: ${cflag}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cflag}")
    endforeach (cflag)
else ()
    message(FATAL_ERROR "no ffmpeg")
endif ()

link_directories(${FFMPEG_LIBRARY_DIRS})
#include_directories(${FFMPEG_INCLUDE_DIRS})

message(STATUS "include: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "libs   : ${FFMPEG_LIBRARY_DIRS}")

set(SOURCES src/main.cpp
        src/mux_title.cpp
        src/mux_title.h
        src/concat.cpp
        src/concat.h
        src/codec/decode.cpp
        src/codec/decode.h
        src/codec/encode.cpp
        src/codec/encode.h
        src/mux/mux_common.h
        src/mux/demux.cpp
        src/mux/demux.h
        src/mux/mux.cpp
        src/mux/mux.h
        src/mux/muxer.cpp
        src/mux/muxer.h
        src/mux/Track.cpp
        src/mux/Track.h
        src/filter/filter_common.cpp
        src/filter/filter_common.h
        src/filter/mix_filter.cpp
        src/filter/mix_filter.h
        src/filter/blur_filter.cpp
        src/filter/blur_filter.h
        src/utils/log.cpp
        src/utils/log.h
        src/utils/io.cpp
        src/utils/io.h
        src/utils/snapshot.cpp
        src/utils/snapshot.h
        src/test/test_muxer.cpp
        src/test/test_muxer.h
        src/test/test_demuxer.cpp
        src/test/test_demuxer.h
        src/test/mux_encode.cpp
        src/test/mux_encode.h
        src/test/resample.cpp
        src/test/resample.h
        src/test/x264.cpp
        src/test/x264.h
        src/test/test_util.cpp
        src/test/test_util.h
        src/test/filter_audio.cpp
        src/test/filter_audio.h
        src/test/get_info.h
        src/test/get_info.cpp)

add_executable(VideoBox ${SOURCES})

target_link_libraries(VideoBox ${FFMPEG_LIBRARIES})
