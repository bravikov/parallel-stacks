cmake_minimum_required(VERSION 4.1.0)

project(threads-merger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(GTest REQUIRED)

set(SDK_PATH $ENV{SDKROOT})
if (NOT SDK_PATH)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

add_compile_options(--sysroot=${SDK_PATH})

add_executable(threads-merger
    main.cpp
    conanfile.txt

)

add_library(mods)
target_sources(mods
  PUBLIC
    FILE_SET CXX_MODULES FILES
    merger.cppm
)

target_link_libraries(threads-merger PRIVATE
  gtest::gtest
  mods
)
