cmake_minimum_required(VERSION 4.2.3)

project(threads-merger LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json for IDEs" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(GTest REQUIRED)

# Поиск системной библиотеки Graphviz
find_package(PkgConfig REQUIRED)

# Попробуем найти через pkg-config
pkg_check_modules(GRAPHVIZ libgvc)

# Если pkg-config не сработал, попробуем найти вручную
if(NOT GRAPHVIZ_FOUND)
    # На macOS с Homebrew
    if(APPLE)
        set(GRAPHVIZ_INCLUDE_DIRS "/opt/homebrew/include/graphviz")
        set(GRAPHVIZ_LIBRARY_DIRS "/opt/homebrew/lib")
        set(GRAPHVIZ_LIBRARIES "gvc")
        set(GRAPHVIZ_FOUND TRUE)
    endif()
endif()

# Graphviz is mandatory for CLI build
if(NOT GRAPHVIZ_FOUND)
    message(FATAL_ERROR "Graphviz not found. Install with 'brew install graphviz' or provide GRAPHVIZ_* variables.")
endif()

set(SDK_PATH $ENV{SDKROOT})
if (NOT SDK_PATH)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

add_compile_options(--sysroot=${SDK_PATH})

add_library(threads-merger-lib STATIC
    merger.hpp
    merger.cpp
    html/html_table.hpp
    html/html_table.cpp
)

target_include_directories(threads-merger-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# Unit tests.

SET(TESTS_DATA_FILES
    tests_data/test-int.dot
    tests_data/test-frame.dot
)

foreach(test_file ${TESTS_DATA_FILES})
    configure_file(
        ${test_file}
        ${test_file}
        COPYONLY
    )
endforeach(test_file ${TESTS_DATA_FILES})

add_executable(threads-merger-tests
    unittests.cpp
)

target_link_libraries(threads-merger-tests PRIVATE
  threads-merger-lib
  gtest::gtest
)

target_include_directories(threads-merger-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# CLI utility. Reads input and prints SVG to stdout.

add_executable(threads-merger-cli
    cli.cpp
)

target_link_libraries(threads-merger-cli PRIVATE
  threads-merger-lib
  ${GRAPHVIZ_LIBRARIES}
)

target_include_directories(threads-merger-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GRAPHVIZ_INCLUDE_DIRS})
target_link_directories(threads-merger-cli PRIVATE ${GRAPHVIZ_LIBRARY_DIRS})
target_compile_options(threads-merger-cli PRIVATE ${GRAPHVIZ_CFLAGS_OTHER})

# WASM module via emcc.
find_program(EMCC_EXECUTABLE emcc)
if(EMCC_EXECUTABLE)
    set(WASM_JS_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/merger.js")
    set(WASM_WASM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/merger.wasm")
    set(WASM_CPP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/html/html_table.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/merger.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/merger-wasm.cpp"
    )
    set(WASM_SOURCE_DEPENDENCIES
        ${WASM_CPP_SOURCES}
        "${CMAKE_CURRENT_SOURCE_DIR}/html/html_table.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/merger.hpp"
    )
    set(WASM_RESULT
        "${WASM_JS_OUTPUT}"
        "${WASM_WASM_OUTPUT}"
    )

    add_custom_command(
        OUTPUT ${WASM_RESULT}
        COMMAND "${EMCC_EXECUTABLE}"
            --std=c++23
            -O2
            -lembind
            ${WASM_CPP_SOURCES}
            -o "${WASM_JS_OUTPUT}"
        DEPENDS
            ${WASM_SOURCE_DEPENDENCIES}
        COMMENT "Building WebAssembly module (merger.js/merger.wasm)"
        VERBATIM
    )

    add_custom_target(threads-merger-wasm ALL
        DEPENDS ${WASM_RESULT}
    )
else()
    message(STATUS "emcc not found. WASM target 'threads-merger-wasm' will be skipped.")
endif()
