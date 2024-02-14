cmake_minimum_required(VERSION 3.16)
project(MPHF LANGUAGES CXX)

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(current-shader-dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

    # compile shader binaries to binary directory
    set(current-output-path ${CMAKE_BINARY_DIR}/${SHADER}.spv)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
            OUTPUT ${current-output-path}
            COMMAND ${GLSLC} --target-env=vulkan1.3 -o ${current-output-path} ${current-shader-path}
            DEPENDS ${current-shader-path}
            IMPLICIT_DEPENDS CXX ${current-shader-dir}
            VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

find_package(Vulkan REQUIRED)
find_package(OpenMP)

# ---------------------------- Library Setup ----------------------------
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(GpuPTHash SHARED ${SOURCE_FILES})
target_compile_features(GpuPTHash INTERFACE cxx_std_17)
target_include_directories(GpuPTHash PUBLIC include)
target_include_directories(GpuPTHash PUBLIC external)
target_include_directories(GpuPTHash PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(GpuPTHash PUBLIC Vulkan::Vulkan)
target_link_libraries(GpuPTHash PUBLIC OpenMP::OpenMP_CXX)

file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
foreach (file ${files})
    get_filename_component(file ${file} NAME)
    add_shader(GpuPTHash shaders/${file})
endforeach ()

file(COPY bucketMappings DESTINATION ${CMAKE_BINARY_DIR}) # include bucket mapping CSV files

# ---------------------------- Benchmarks ----------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(DEV dev/main.cpp)
    target_link_libraries(DEV PUBLIC GpuPTHash)
endif()
