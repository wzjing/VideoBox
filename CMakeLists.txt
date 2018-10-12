cmake_minimum_required(VERSION 3.10)
project(VideoBox)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

link_directories(${CMAKE_SOURCE_DIR}/libs/ffmpeg/lib ${CMAKE_SOURCE_DIR}/libs/x264/lib)

include_directories(${CMAKE_SOURCE_DIR}/libs/ffmpeg/include ${CMAKE_SOURCE_DIR}/libs/x264/include)

add_executable(VideoBox main.cpp test.h test.cpp)

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
