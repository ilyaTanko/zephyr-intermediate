cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(calculator_nvs_unit_test)

target_sources(app PRIVATE
    src/nvs_fake.c
    src/test_basic.c
    src/test_error_paths.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/modules/calculator_nvs/src/calculator_nvs.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/modules/calculator_nvs/include
    include
)
