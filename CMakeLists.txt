cmake_minimum_required(VERSION 3.10)
project(VideoBox)

include(FindPkgConfig)

add_definitions(DEBUG)

set(CMAKE_C_FLAGS "-D__STDC_FORMAT_MACROS")

if (${PKG_CONFIG_FOUND})
    message(STATUS "found pkg-config ${PKG_CONFIG_VERSION_STRING} in ${PKG_CONFIG_EXECUTABLE}")
else ()
    message(FATAL_ERROR "no pkg-config found, please install pkg-config and add libraries")
endif ()

pkg_check_modules(FFMPEG libavcodec libavutil libavformat libavfilter libavdevice libswscale)

if (FFMPEG_FOUND)
    foreach (cflag ${FFMPEG_CFLAGS})
        message("cflags: ${cflag}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
    endforeach (cflag)
    foreach (ldflag ${FFMPEG_LDFLAGS})
        message("ldflag: ${ldflag}")
    endforeach (ldflag)
endif ()

set(SOURCES src/main.c
        src/muxing/demuxing.c
        src/muxing/muxing.c
        src/codec/decode.c
        src/codec/encode.c
        src/image/scale.c
        src/image/scale.h src/muxing/concat.c src/muxing/concat.h src/utils/snapshot.c src/utils/snapshot.h src/utils/log.h)

add_executable(VideoBox ${SOURCES})

target_link_libraries(VideoBox ${FFMPEG_LDFLAGS})
