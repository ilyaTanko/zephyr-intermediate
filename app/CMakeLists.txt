cmake_minimum_required(VERSION 3.20.0)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(ENV{CCACHE_BASEDIR} "${CMAKE_SOURCE_DIR}")
    set(ENV{CCACHE_DIR} "${CMAKE_SOURCE_DIR}/../.cache/ccache")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

find_package(Zephyr REQUIRED)
project(my_zephyr_app)

target_sources(app PRIVATE src/main.c)
