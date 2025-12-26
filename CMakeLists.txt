cmake_minimum_required(VERSION 3.16)
project(beater VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build unit tests" ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# JACK (mandatory)
pkg_check_modules(JACK REQUIRED jack)

# libsndfile (optional for now, required for Phase 2)
pkg_check_modules(SNDFILE sndfile)

# Qt6 (optional, required for UI Phase 5)
find_package(Qt6 COMPONENTS Core Widgets)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    set(BUILD_UI ON)
    message(STATUS "Qt6 found - UI will be built")
else()
    set(BUILD_UI OFF)
    message(STATUS "Qt6 not found - UI will not be built (Phase 1 testing only)")
endif()

# Add external dependencies (header-only libs)
add_subdirectory(external)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
add_subdirectory(src)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
