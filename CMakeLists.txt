cmake_minimum_required(VERSION 3.16)

project(order_book_hft
    VERSION 0.1
    LANGUAGES CXX
)

# -------------------------------
# Compiler settings
# -------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# -------------------------------
# Compiler flags (HFT-style)
# -------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

# Performance flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O3
        -march=native
        -mtune=native
        -fno-omit-frame-pointer
    )
endif()

# -------------------------------
# Executable
# -------------------------------
add_executable(order_book
    src/main.cpp
)

target_include_directories(order_book
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# -------------------------------
# Threading
# -------------------------------
find_package(Threads REQUIRED)
target_link_libraries(order_book
    PRIVATE
        Threads::Threads
)

# -------------------------------
# Linux-specific (optional but common in HFT)
# -------------------------------
if(UNIX AND NOT APPLE)
    target_link_libraries(order_book
        PRIVATE
            rt
    )
endif()

