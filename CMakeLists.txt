cmake_minimum_required(VERSION 3.10)
project(VideoBox)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

add_executable(VideoBox src/main.c src/util/screenshot.h src/util/screenshot.c)

target_link_libraries(VideoBox
        avdevice
        avfilter
        avformat
        avcodec
        swscale
        swresample
        postproc
        avutil
        x264
        pthread
        bsd
        c
        z
        m
        dl
        )
