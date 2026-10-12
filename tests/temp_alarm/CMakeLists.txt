cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(temp_alarm_unit_test)

target_sources(app PRIVATE
    src/sensor_fake.c
    src/test_basic.c
    src/test_error_paths.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/modules/temp_alarm/src/temp_alarm.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/modules/temp_alarm/include
    include
)
