cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ASM is required for shaders.
project(GLASS C ASM)

# Setup baremetal.
if (CTR_BAREMETAL)
    include(BM/ARM11)
endif()

option(GLASS_COMPILE_EXAMPLES "Enable examples compilation" OFF)

# Setup dependencies.
CPMAddPackage("gh:kynex7510/KYGX#3f257b9")

CPMAddPackage(
    URI "gh:kynex7510/RIP#bd9af06"
    OPTIONS "RIP_BACKEND kygx"
)

# Setup library.
set(GLASS_SOURCES
    Source/Base/Context.c
    Source/Base/GLASS.c
    Source/Base/Math.c
    Source/Base/Memory.c
    Source/Base/TexManager.c
    Source/Base/Types.c
    Source/Common/Attribs.c
    Source/Common/Buffers.c
    Source/Common/Combiners.c
    Source/Common/Effects.c
    Source/Common/Framebuffer.c
    Source/Common/Rendering.c
    Source/Common/Shaders.c
    Source/Common/State.c
    Source/Common/Texture.c
    Source/Common/Uniforms.c
    Source/Platform/GFX.c
    Source/Platform/GPU.c
)

add_library(GLASSv2 STATIC ${GLASS_SOURCES})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(GLASSv2 PRIVATE GLASS_NO_MERCY)
endif()

target_include_directories(GLASSv2 PUBLIC Include PRIVATE Source)
target_link_libraries(GLASSv2 PUBLIC kygx rip)
target_compile_options(GLASSv2 PRIVATE -Wall -Werror)

install(TARGETS GLASSv2)
install(DIRECTORY Include/ DESTINATION include)

if (GLASS_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()