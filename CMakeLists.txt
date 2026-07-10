cmake_minimum_required(VERSION 3.16)
project(cod2_iwi_gallery CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(cod2-iwi-gallery
    src/main.cpp
    src/vfs.cpp
    src/image.cpp
    src/categorize_tex.cpp
    src/gallery_tex.cpp
    third_party/miniz.c
)
# No GPU/GL: this tool is pure CPU decode -> downscale -> JPEG.
