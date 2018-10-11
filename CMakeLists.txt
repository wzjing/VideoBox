cmake_minimum_required(VERSION 3.10)
project(VideoBox C)

set(CMAKE_C_STANDARD 99)

link_directories(${CMAKE_SOURCE_DIR}/libs/ffmpeg/lib)

include_directories(${CMAKE_SOURCE_DIR}/libs/ffmpeg/include)

add_executable(VideoBox main.c test.h test.c)

target_link_libraries(VideoBox
        avdevice
        avfilter
        avformat
        avcodec
        swscale
        swresample
        avutil
        pthread
        bsd
        c
        z
        m
        )
