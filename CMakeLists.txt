cmake_minimum_required(VERSION 3.20)
project(engine_2d C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(HAJIMU_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../jp/include")
message(STATUS "HAJIMU_INCLUDE_DIR = ${HAJIMU_INCLUDE_DIR}")

add_library(engine_2d SHARED
    src/eng_phys.c
    src/eng_map.c
    src/plugin.c
)

target_include_directories(engine_2d PRIVATE
    ${HAJIMU_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

if(UNIX AND NOT APPLE)
    target_link_libraries(engine_2d PRIVATE m)
endif()

set_target_properties(engine_2d PROPERTIES
    OUTPUT_NAME "engine_2d"
    SUFFIX ".hjp"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
)
