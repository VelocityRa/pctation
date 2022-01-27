### Initial CMake set-up
cmake_minimum_required(VERSION 3.12.4)

# Work around a bug with old MSVC
if(${CMAKE_VERSION} VERSION_LESS 3.14)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.14)
endif()

if(NOT WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    link_libraries(stdc++fs)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src)
include_directories(external)

# vcpkg setup
set(VCPKG_ROOT "$C:/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Project definition
project(pctation VERSION 0.1
                  DESCRIPTION "PlayStation emulator"
                  LANGUAGES CXX)

# Global CMake options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

### vcpkg dependencies

# spdlog
find_package(spdlog CONFIG REQUIRED)
target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
                                                    SPDLOG_NO_THREAD_ID=1
                                                    SPDLOG_NO_DATETIME=1
                                                    SPDLOG_NO_NAME=1
                                                    SPDLOG_NO_ATOMIC_LEVELS=1
                                                    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)

# GSL
find_path(GSL_INCLUDE_DIR gsl-lite.hpp)
if(GSL_INCLUDE_DIR STREQUAL "GSL_INCLUDE_DIR-NOTFOUND")
    message(FATAL_ERROR "Could not find Guidelines Support Library.")
endif()
include_directories(${GSL_INCLUDE_DIR})

# SDL2
find_package(SDL2 CONFIG REQUIRED)

# fmt
find_package(fmt CONFIG REQUIRED)

# imgui
find_package(imgui CONFIG REQUIRED)

# glbinding
find_package(glbinding CONFIG REQUIRED)

# glm
find_package(glm CONFIG REQUIRED)

### Add source tree
add_subdirectory(src)

### Add externals directory for additional externals
add_subdirectory(external)

### Main executable
add_executable(pctation src/main/main.cpp)
target_link_libraries(pctation PRIVATE main)

set_property(TARGET pctation PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pctation)

add_custom_command(
    TARGET pctation
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data"
)
