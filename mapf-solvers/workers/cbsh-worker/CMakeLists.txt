add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../solvers/CBSH2-RTC ${CMAKE_CURRENT_BINARY_DIR}/CBSH2-RTC)

add_executable(mapf-solver-worker-cbsh
        src/main.cpp
)

target_link_libraries(mapf-solver-worker-cbsh
        PRIVATE
        mapf::common
        mapf::solvers::protocol
        CBSH2_Library
)