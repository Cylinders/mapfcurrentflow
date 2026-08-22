cmake_minimum_required(VERSION 3.21)

project(bcp-mapf LANGUAGES C CXX)

include(CheckCXXCompilerFlag)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost REQUIRED COMPONENTS program_options system filesystem)

find_library(LIBM m)
if (NOT LIBM)
    set(LIBM "")
endif ()

set(BCP_MAPF_SOURCE_FILES
        bcp/branching/branching_rule.cpp
        bcp/branching/fractional.cpp
        bcp/branching/length_branching_constraint.cpp
        bcp/branching/pseudosolution.cpp
        bcp/branching/vertex_branching_constraint.cpp

        bcp/constraints/agent_wait_edge.cpp
        bcp/constraints/corridor.cpp
        bcp/constraints/edgetime.cpp
        bcp/constraints/exit_entry.cpp
        bcp/constraints/five_edge.cpp
        bcp/constraints/four_edge.cpp
        bcp/constraints/nodetime.cpp
        bcp/constraints/preprocessing.cpp
        bcp/constraints/rectangle_knapsack.cpp
        bcp/constraints/rectangle.cpp
        bcp/constraints/six_edge.cpp
        bcp/constraints/step_aside.cpp
        bcp/constraints/target.cpp
        bcp/constraints/three_vertex.cpp
        bcp/constraints/two_edge.cpp
        bcp/constraints/two_vertex.cpp
        bcp/constraints/vertex_four_edge.cpp
        bcp/constraints/wait_delay.cpp

        bcp/heuristics/prioritized_planning.cpp

        bcp/pricing/astar.cpp
        bcp/pricing/distance_heuristic.cpp
        bcp/pricing/pricer.cpp

        bcp/problem/bcp_instance.cpp
        bcp/problem/map.cpp
        bcp/problem/output.cpp
        bcp/problem/problem.cpp
        bcp/problem/reader.cpp
        bcp/problem/variable_data.cpp

        bcp/types/memory_pool.cpp
)

add_library(bcp-mapf STATIC ${BCP_MAPF_SOURCE_FILES} adapter/bcp_adapter.cpp adapter/heuristics/seeded_solution.cpp)

target_include_directories(bcp-mapf
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/adapter
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/bcp
)

find_path(SCIP_INCLUDE_DIR
        NAMES scip/scip.h
        HINTS "${SCIP_DIR}/scip/src" "$ENV{SCIP_DIR}/scip/src"
)

find_path(SCIP_CONFIG_DIR
        NAMES scip/config.h
        HINTS "${SCIP_DIR}/build/scip" "$ENV{SCIP_DIR}/build/scip"
)

find_library(SCIP_LIBRARY
        NAMES scip
        HINTS "${SCIP_DIR}/build/lib" "$ENV{SCIP_DIR}/build/lib"
)

if (SCIP_INCLUDE_DIR AND SCIP_CONFIG_DIR AND SCIP_LIBRARY)
    target_include_directories(bcp-mapf PRIVATE
            ${SCIP_INCLUDE_DIR}
            ${SCIP_CONFIG_DIR}
    )

    message(STATUS "Using SCIP binary at ${SCIP_LIBRARY}")
else ()
    find_path(GUROBI_INCLUDE_DIRS
            NAMES gurobi_c.h
            HINTS
            "${GUROBI_DIR}/include"
            "${GUROBI_DIR}/*/include"
            "$ENV{GUROBI_DIR}/include"
            "$ENV{GUROBI_DIR}/*/include"
            "$ENV{GUROBI_HOME}/include"
    )

    find_library(GUROBI_LIBRARY
            NAMES gurobi120 gurobi110 gurobi100 gurobi95 gurobi91 gurobi90 gurobi81 gurobi80 gurobi75 gurobi70
            HINTS
            "${GUROBI_DIR}/lib"
            "${GUROBI_DIR}/*/lib"
            "$ENV{GUROBI_DIR}/lib"
            "$ENV{GUROBI_DIR}/*/lib"
            "$ENV{GUROBI_HOME}/lib"
    )

    if (GUROBI_INCLUDE_DIRS AND GUROBI_LIBRARY)
        set(LPS grb CACHE STRING "options for LP solver")
        target_include_directories(bcp-mapf PRIVATE ${GUROBI_INCLUDE_DIRS})
        message(STATUS "Using Gurobi at ${GUROBI_LIBRARY} as LP solver")
    endif ()

    if (NOT GUROBI_INCLUDE_DIRS OR NOT GUROBI_LIBRARY)
        find_path(CPLEX_INCLUDE_DIR
                NAMES cplex.h
                HINTS "${CPLEX_DIR}/include/ilcplex" "$ENV{CPLEX_DIR}/include/ilcplex"
        )

        find_library(CPLEX_LIBRARY
                NAMES cplex
                HINTS "${CPLEX_DIR}/lib/*/static_pic" "$ENV{CPLEX_DIR}/lib/*/static_pic"
        )

        if (CPLEX_INCLUDE_DIR AND CPLEX_LIBRARY)
            set(LPS cpx CACHE STRING "options for LP solver")
            target_include_directories(bcp-mapf PRIVATE ${CPLEX_INCLUDE_DIR})
            message(STATUS "Using CPLEX at ${CPLEX_LIBRARY} as LP solver")
        endif ()
    endif ()

    if ((NOT GUROBI_INCLUDE_DIRS OR NOT GUROBI_LIBRARY) AND (NOT CPLEX_INCLUDE_DIR OR NOT CPLEX_LIBRARY))
        set(LPS spx CACHE STRING "options for LP solver")

        set(CONF_INCLUDE_DIRS ../soplex/src/)
        configure_file(
                scipoptsuite-9.2.0/soplex/soplex-config.cmake.in
                "${CMAKE_BINARY_DIR}/soplex/soplex-config.cmake"
                @ONLY
        )

        set(SOPLEX_DIR ${CMAKE_BINARY_DIR}/soplex)
        add_subdirectory(scipoptsuite-9.2.0/soplex EXCLUDE_FROM_ALL)
        message(STATUS "Using SoPlex as LP solver")
    endif ()

    set(WITH_SCIPDEF ON)
    set(TPI none)
    set(tpisources tpi/tpi_none.c)
    set(THREAD_LIBRARIES "")
    set(TPI_NONE ON)
    set(GMP OFF)
    set(IPOPT OFF)
    set(PAPILO OFF)
    set(ZIMPL OFF)

    configure_file(
            scipoptsuite-9.2.0/scip/src/scip/config.h.in
            "${CMAKE_BINARY_DIR}/scip/config.h"
            @ONLY
    )

    target_include_directories(bcp-mapf PRIVATE
            ${CMAKE_BINARY_DIR}
            scipoptsuite-9.2.0/scip/src
    )

    add_subdirectory(scipoptsuite-9.2.0/scip EXCLUDE_FROM_ALL)

    set(SCIP_LIBRARY libscip)
endif ()

add_subdirectory(fmt EXCLUDE_FROM_ALL)

target_include_directories(bcp-mapf PRIVATE
        fmt/include
        cxxopts/include
        robin-hood-hashing/src/include
        scipoptsuite-9.2.0/scip/src/scip
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(bcp-mapf PRIVATE
        Boost::headers
        fmt::fmt-header-only
        ${SCIP_LIBRARY}
        ${LIBM}
)

target_compile_definitions(bcp-mapf PRIVATE
        USE_RESERVATION_TABLE
        USE_ASTAR_SOLUTION_CACHING
        USE_WAITEDGE_CONFLICTS
        USE_RECTANGLE_KNAPSACK_CONFLICTS
        USE_CORRIDOR_CONFLICTS
        USE_WAITCORRIDOR_CONFLICTS
        USE_WAITDELAY_CONFLICTS
        USE_EXITENTRY_CONFLICTS
        USE_TWOEDGE_CONFLICTS
        USE_WAITTWOEDGE_CONFLICTS
        USE_AGENTWAITEDGE_CONFLICTS
        USE_GOAL_CONFLICTS
        USE_BEST_FIRST_NODE_SELECTION
)

target_compile_options(bcp-mapf PRIVATE
        -Wall
        -Wextra
        -Wignored-qualifiers
        -Werror=return-type
        -Wno-sign-compare
)

check_cxx_compiler_flag("-march=native" MARCH_NATIVE)
if (MARCH_NATIVE)
    target_compile_options(bcp-mapf PRIVATE -march=native)
endif ()

target_compile_features(bcp-mapf PRIVATE cxx_std_17)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(bcp-mapf PRIVATE DEBUG)
    message(STATUS "BCP-MAPF compiled in debug mode")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(bcp-mapf PRIVATE NDEBUG)
    target_compile_options(bcp-mapf PRIVATE -Og -funroll-loops -fstrict-aliasing)
    message(STATUS "BCP-MAPF compiled in release with debug info mode")
else ()
    target_compile_definitions(bcp-mapf PRIVATE NDEBUG)
    target_compile_options(bcp-mapf PRIVATE -O3 -funroll-loops -fstrict-aliasing)
    message(STATUS "BCP-MAPF compiled in release mode")
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(bcp-mapf PRIVATE dl)
endif ()

target_link_libraries(bcp-mapf
        PUBLIC
        mapf::common
)
