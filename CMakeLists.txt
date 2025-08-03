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

option(GLASS_ES_1_1 "Enable ES 1.1 support" OFF)
option(GLASS_COMPILE_EXAMPLES "Enable examples compilation" OFF)

# Setup dependencies.
CPMAddPackage("gh:kynex7510/KYGX#e704045")

CPMAddPackage(
    URI "gh:kynex7510/RIP#93c244d"
    OPTIONS "RIP_BACKEND kygx"
)

CPMAddPackage(
    URI "gh:kynex7510/kazmath_3ds#e3f3d3b"
    OPTIONS "KAZMATH_BUILD_GL_UTILS OFF"
)

# Setup library.
set(GLASS_SOURCES
    Source/Base/Context.c
    Source/Base/GLASS.c
    Source/Base/Math.c
    Source/Base/MathCTRU.c
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

if (GLASS_ES_1_1)
    set(GLASS_SOURCES
        ${GLASS_SOURCES}
        Source/V1/Matrix.c
    )
endif()

add_library(GLASS STATIC ${GLASS_SOURCES})

if (GLASS_ES_1_1)
    target_compile_definitions(GLASS PRIVATE GLASS_ES_1_1)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(GLASS PRIVATE GLASS_NO_MERCY)
endif()

target_include_directories(GLASS PUBLIC Include PRIVATE Source)
target_link_libraries(GLASS PUBLIC kygx rip kazmath)
target_compile_options(GLASS PRIVATE -Wall -Werror)

install(TARGETS GLASS)
install(DIRECTORY Include/ DESTINATION include)

if (GLASS_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()