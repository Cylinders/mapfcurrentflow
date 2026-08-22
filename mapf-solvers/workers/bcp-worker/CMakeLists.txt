add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../solvers/bcp-mapf ${CMAKE_CURRENT_BINARY_DIR}/bcp-mapf)

add_executable(mapf-solver-worker-bcp
        src/main.cpp
)

target_link_libraries(mapf-solver-worker-bcp
        PRIVATE
        mapf::common
        mapf::solvers::protocol
        bcp-mapf
)