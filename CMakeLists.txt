cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMake/CPM.cmake)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(GLASS C)

option(GLASS_COMPILE_EXAMPLES "Enable examples compilation" OFF)

# Setup dependencies.
CPMAddPackage("gh:kynex7510/KYGX#302d8d0")
CPMAddPackage("gh:kynex7510/RIP#4b0906b")

# Setup library.
set(GLASS_SOURCES
    Base/Context.c
    Base/Flip.c
    Base/Format.c
    Base/GLASS.c
    Base/Memory.c
    Base/Texture.c
    Base/Tiling.c
    Base/Types.c
    Base/Utility.c
    Common/Attribs.c
    Common/Buffers.c
    Common/Combiners.c
    Common/Effects.c
    Common/Framebuffer.c
    Common/Rendering.c
    Common/Shaders.c
    Common/State.c
    Common/Tex3DS.c
    Common/TexGL.c
    Common/Uniforms.c
    Platform/GPU.c
    Platform/GX.c
)

add_library(glass STATIC ${GLASS_SOURCES})
target_include_directories(glass PUBLIC Include)
target_compile_options(glass PRIVATE -Wall -Werror)

install(TARGETS glass)
install(DIRECTORY Include/ DESTINATION include)

if (GLASS_COMPILE_EXAMPLES)
    #add_subdirectory(Examples)
endif()