cmake_minimum_required(VERSION 3.20)

project(dallas LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ORT_DIR ${CMAKE_SOURCE_DIR}/external/onnxruntime)

add_subdirectory(mapf-common)
add_subdirectory(mapf-solvers)
add_subdirectory(mapf-benchmark)
