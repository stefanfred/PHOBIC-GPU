cmake_minimum_required(VERSION 3.19...3.31)
project(GpuPTHash LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()
MESSAGE(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

if((CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_compile_options(-march=native)
endif()

# ---------------------------- Dependencies ----------------------------
if(NOT TARGET tlx)
    set(TLX_INSTALL_INCLUDE_DIR tlx CACHE PATH "Workaround for TLX breaking the first cmake call")
    add_subdirectory(external/tlx)
endif()

# ---------------------------- Library Setup ----------------------------
function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(current-shader-dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

    # compile shader binaries to binary directory
    set(current-output-path ${CMAKE_BINARY_DIR}/${SHADER}.spv)

    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
            OUTPUT ${current-output-path}
            COMMAND ${GLSLC} --target-env=vulkan1.3 --target-spv=spv1.5 -o ${current-output-path} ${current-shader-path}
            DEPENDS ${current-shader-path}
            IMPLICIT_DEPENDS CXX ${current-shader-dir}
            VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(GpuPTHash SHARED ${SOURCE_FILES})
target_compile_features(GpuPTHash INTERFACE cxx_std_17)
target_include_directories(GpuPTHash SYSTEM PUBLIC include)
target_include_directories(GpuPTHash SYSTEM PUBLIC external)
target_include_directories(GpuPTHash SYSTEM PUBLIC external/xxHash)
target_include_directories(GpuPTHash SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(GpuPTHash PUBLIC Vulkan::Vulkan)
target_link_libraries(GpuPTHash PUBLIC OpenMP::OpenMP_CXX)

file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
foreach (file ${files})
    get_filename_component(file ${file} NAME)
    add_shader(GpuPTHash shaders/${file})
endforeach ()

add_subdirectory(external/essentials)
target_link_libraries(GpuPTHash PUBLIC ESSENTIALS)

# ---------------------------- Benchmarks ----------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(EXAMPLE example.cpp)
    target_link_libraries(EXAMPLE PUBLIC GpuPTHash)

    add_executable(BENCHMARK benchmark.cpp)
    target_link_libraries(BENCHMARK PUBLIC GpuPTHash tlx)
endif()
