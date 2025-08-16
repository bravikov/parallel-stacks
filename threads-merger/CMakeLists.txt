cmake_minimum_required(VERSION 4.1.0)

project(threads-merger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

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
        set(GRAPHVIZ_LIBRARIES "gvc" "cgraph" "cdt")
        set(GRAPHVIZ_FOUND TRUE)
    endif()
endif()

# Проверяем, что Graphviz найден
if(NOT GRAPHVIZ_FOUND)
    message(FATAL_ERROR "Graphviz не найден. Установите его через: brew install graphviz")
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

add_executable(threads-merger
    main.cpp

    merger.hpp
    merger.cpp

    html/html_table.hpp
    html/html_table.cpp

    conanfile.txt
)

target_link_libraries(threads-merger PRIVATE
  gtest::gtest
  ${GRAPHVIZ_LIBRARIES}
)

target_include_directories(threads-merger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GRAPHVIZ_INCLUDE_DIRS})
target_link_directories(threads-merger PRIVATE ${GRAPHVIZ_LIBRARY_DIRS})
target_compile_options(threads-merger PRIVATE ${GRAPHVIZ_CFLAGS_OTHER})

# CLI utility target (no GTest). Builds a console app that reads input and prints SVG to stdout.
add_executable(threads-merger-cli
    cli.cpp

    merger.hpp
    merger.cpp

    html/html_table.hpp
    html/html_table.cpp
)

target_link_libraries(threads-merger-cli PRIVATE
  ${GRAPHVIZ_LIBRARIES}
)

target_include_directories(threads-merger-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GRAPHVIZ_INCLUDE_DIRS})
target_link_directories(threads-merger-cli PRIVATE ${GRAPHVIZ_LIBRARY_DIRS})
target_compile_options(threads-merger-cli PRIVATE ${GRAPHVIZ_CFLAGS_OTHER})
