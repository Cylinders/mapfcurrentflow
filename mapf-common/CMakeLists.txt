find_package(OpenCV REQUIRED)

add_library(mapf-common STATIC
        src/map_reader.cpp
        src/map_writer.cpp
        src/scenario_reader.cpp
        src/scenario_writer.cpp
        src/solution_reader.cpp
        src/solution_writer.cpp
        src/minimal_paths.cpp
        src/meta/mapfaster_encoder.cpp
        src/meta/mapfaster_valuator.cpp
)

add_library(mapf::common ALIAS mapf-common)

target_include_directories(mapf-common
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${ORT_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_directories(mapf-common
        PUBLIC
        ${ORT_DIR}/lib
)

target_link_libraries(mapf-common
        PUBLIC
        onnxruntime
        PRIVATE
        ${OpenCV_LIBS}
)

target_compile_features(mapf-common
        PUBLIC
        cxx_std_20
)