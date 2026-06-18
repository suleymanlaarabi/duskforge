cmake_minimum_required(VERSION 3.20)

project(hello_flecs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DUSKLIGHT_SDK_DIR)
    set(DUSKLIGHT_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH "Dusklight repository root")
endif ()

add_library(hello_flecs SHARED hello_flecs.cpp)
target_include_directories(hello_flecs PRIVATE
    "${DUSKLIGHT_SDK_DIR}/include"
    "${DUSKLIGHT_SDK_DIR}/libs/flecs")
set_target_properties(hello_flecs PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hello_flecs")

if (APPLE)
    target_link_options(hello_flecs PRIVATE "-undefined" "dynamic_lookup")
endif ()

