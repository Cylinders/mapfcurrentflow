add_subdirectory("workers/cbsh-worker")
add_subdirectory("workers/bcp-worker")

# protocol
add_library(mapf-solvers-protocol STATIC src/protocol.cpp)

target_include_directories(mapf-solvers-protocol PUBLIC include)

target_link_libraries(mapf-solvers-protocol PUBLIC mapf::common)

add_library(mapf::solvers::protocol ALIAS mapf-solvers-protocol)

# solver api
add_library(mapf-solvers STATIC src/solver.cpp src/worker.cpp)

target_include_directories(mapf-solvers PUBLIC include)
target_link_libraries(mapf-solvers PUBLIC mapf::common PRIVATE mapf-solvers-protocol)

add_library(mapf::solvers ALIAS mapf-solvers)
