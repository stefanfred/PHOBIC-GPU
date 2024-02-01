cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
project(MPHF)
 

function(add_shader TARGET SHADER)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})   
    set(current-shader-dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)   

    
    # compile shader binaries to binary directory
    set(current-output-path ${CMAKE_BINARY_DIR}/${SHADER}.spv)
    

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
           OUTPUT ${current-output-path}
           COMMAND glslc --target-env=vulkan1.3 -o ${current-output-path} ${current-shader-path}
           DEPENDS ${current-shader-path}
           IMPLICIT_DEPENDS CXX ${current-shader-dir}
           VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

find_package(Vulkan REQUIRED)

find_package(OpenMP)



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lm -ldl")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2 -msse4.2")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")


## include res
file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

#file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")
#foreach(file ${files})
#    get_filename_component(file ${file} NAME)
#    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shaders/${file} ${CMAKE_BINARY_DIR}/shaders/${file} COPYONLY)
#endforeach()


file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(DEV ${SOURCE_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/dev/main.cpp")


file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
foreach(file ${files})
    get_filename_component(file ${file} NAME)
    add_shader(DEV shaders/${file})
endforeach()

## include project include files
target_include_directories(DEV PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(DEV PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_include_directories(DEV PUBLIC "${CMAKE_SOURCE_DIR}/external")

## include VULKAN include files and libraries

#if(RENDERDOC_PATH)
#    target_include_directories(${PROJECT_NAME} PRIVATE ${RENDERDOC_PATH})
#    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_RENDERDOC)
#endif()


target_include_directories(DEV PRIVATE ${Vulkan_INCLUDE_DIRS} )
target_link_libraries(DEV PRIVATE Vulkan::Vulkan)
target_link_libraries(DEV PUBLIC OpenMP::OpenMP_CXX)



## include all source files in src/ directory
#file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB TEST_SOURCES test/test_*.cpp)
foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component (TEST_SRC_NAME ${TEST_SRC} NAME_WE) # without extension
    add_executable(${TEST_SRC_NAME} ${TEST_SRC} ${SOURCE_FILES} )
    add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})

    file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
    foreach(file ${files})
        get_filename_component(file ${file} NAME)
        add_shader(${TEST_SRC_NAME} shaders/${file})
    endforeach()

    ## include project include files
    target_include_directories(${TEST_SRC_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include")
    target_include_directories(${TEST_SRC_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/src")
    target_include_directories(${TEST_SRC_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external")

    target_include_directories(${TEST_SRC_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS} )
    target_link_libraries(${TEST_SRC_NAME} PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(${TEST_SRC_NAME} PRIVATE Vulkan::Vulkan)
endforeach(TEST_SRC)
