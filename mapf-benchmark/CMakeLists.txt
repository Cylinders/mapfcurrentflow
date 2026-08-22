find_package(OpenCV REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(mapf-benchmark
        src/main.cpp
        src/manifest.cpp
)

## make sure the workers exist
add_dependencies(mapf-benchmark
        mapf-solver-worker-cbsh
        mapf-solver-worker-bcp
)

target_compile_definitions(mapf-benchmark PRIVATE
        MAPF_BENCHMARK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        MAPF_CBSH_WORKER_PATH="$<TARGET_FILE:mapf-solver-worker-cbsh>"
        MAPF_BCP_WORKER_PATH="$<TARGET_FILE:mapf-solver-worker-bcp>"
)
##

target_include_directories(mapf-benchmark PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(mapf-benchmark PRIVATE
        ${OpenCV_LIBS}
        onnxruntime
        nlohmann_json::nlohmann_json
        mapf::common
        mapf::solvers
)

target_link_libraries(mapf-benchmark PRIVATE mapf::solvers mapf::common Threads::Threads)
