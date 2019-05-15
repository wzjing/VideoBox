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
        src/codec/decode.cpp
        src/codec/encode.cpp
        src/mux/mux_common.h
        src/mux/demux.cpp
        src/mux/mux.cpp
        src/mux/concat.cpp
        src/utils/snapshot.cpp
        src/utils/io.cpp
        src/demux_decode.hpp
        src/mux_encode.hpp
        src/multi-mux.hpp
        src/audio_resample.hpp
        src/audio_filter.hpp
        src/video_filter.hpp
        src/video_encode_x264.hpp
        src/test/test_io.hpp
        src/mux/muxer.h
        src/mux/muxer.cpp
        src/test/test_mux.cpp
        src/mux/Track.cpp
        src/mux/Track.h src/filter/mix_filter.cpp src/filter/mix_filter.h src/filter/blur_filter.cpp src/filter/blur_filter.h src/filter/filter_common.cpp src/filter/filter_common.h)

add_executable(VideoBox ${SOURCES})

target_link_libraries(VideoBox ${FFMPEG_LIBRARIES})
