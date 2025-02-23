cmake_minimum_required(VERSION 3.25) 
project(Isaac CXX)               
set(CMAKE_CXX_STANDARD 17)            
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include("${CMAKE_SOURCE_DIR}/../../addons/prism/cmake/prism.cmake" REQUIRED)

# Add source/header files to project
file(GLOB_RECURSE SOURCES ../*.cpp SOURCES ../*.h SOURCES ../*.rc)
list(FILTER SOURCES EXCLUDE REGEX ".*web.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_link_options(/NODEFAULTLIB:libcmt.lib)
add_link_options(/IGNORE:4099,4286,4098)
add_link_options(/OPT:REF)
add_link_options(/OPT:ICF)

# Define exe
add_executable(Isaac ${SOURCES})

# Link with libraries
target_link_libraries(Isaac prism)
target_link_libraries(Isaac SDL2)
target_link_libraries(Isaac SDL2main)
target_link_libraries(Isaac SDL2_image)
target_link_libraries(Isaac SDL2_mixer)
target_link_libraries(Isaac SDL2_ttf)
target_link_libraries(Isaac enet)
target_link_libraries(Isaac freetype)
target_link_libraries(Isaac glew)
target_link_libraries(Isaac curl)
target_link_libraries(Isaac libjpeg)
target_link_libraries(Isaac libpng)
target_link_libraries(Isaac libwebp)
target_link_libraries(Isaac zstd)
target_link_libraries(Isaac zlib)
target_link_libraries(Isaac crypt32)
target_link_libraries(Isaac wldap32)
target_link_libraries(Isaac ws2_32)
target_link_libraries(Isaac winmm)
target_link_libraries(Isaac OpenGL32)
target_link_libraries(Isaac version)

# Define preprocessor definitions
target_compile_definitions(Isaac PUBLIC UNICODE)
target_compile_definitions(Isaac PUBLIC _UNICODE)

set_property(TARGET Isaac PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(Isaac PRIVATE /Gy)

set_target_properties(Isaac PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../assets)
set_target_properties(Isaac PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../assets)

# Copy over DLLs
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/../../addons/prism/windows/vs17/DLL/*.dll")
foreach(DLL ${DLLS})
  add_custom_command(TARGET Isaac POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLL}
    $<TARGET_FILE_DIR:Isaac>)
endforeach()

# Optimize for release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(Isaac PRIVATE /O2)
endif()
