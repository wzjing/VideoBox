cmake_minimum_required(VERSION 3.10)
project(VideoBox)

#set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include(FindPkgConfig)

add_definitions(-DDEBUG -D__STDC_FORMAT_MACROS)

#set(CMAKE_C_FLAGS "-fblocks")

if (${PKG_CONFIG_FOUND})
    message(STATUS "found pkg-config ${PKG_CONFIG_VERSION_STRING} in ${PKG_CONFIG_EXECUTABLE}")
else ()
    message(FATAL_ERROR "no pkg-config found, please install pkg-config and add libraries")
endif ()

pkg_check_modules(FFMPEG libavcodec libavutil libavformat libavfilter libavdevice libswscale libswresample)

if (FFMPEG_FOUND)
    foreach (cflag ${FFMPEG_CFLAGS})
        message(STATUS "cflags: ${cflag}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
        set(CMAKE_CXX_FLAGS )
    endforeach (cflag)
    foreach (ldflag ${FFMPEG_LDFLAGS})
        message(STATUS "ldflag: ${ldflag}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ldflag}")
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${ldflag}")
    endforeach (ldflag)
endif ()

set(SOURCES src/main.cpp
        src/muxing/demuxing.cpp
        src/muxing/muxing.cpp
        src/codec/decode.cpp
        src/codec/encode.cpp
        src/muxing/concat.cpp
        src/muxing/concat.h
        src/utils/snapshot.cpp
        src/utils/snapshot.h
        src/utils/log.h
        src/demux_decode.cpp
        src/demux_decode.h
        src/mux_encode.cpp
        src/mux_encode.h
        src/utils/io.cpp
        src/utils/io.h)

add_executable(VideoBox ${SOURCES})

#target_link_libraries(VideoBox ${FFMPEG_LDFLAGS} )
