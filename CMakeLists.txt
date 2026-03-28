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
CPMAddPackage("gh:kynex7510/KYGX#11adef4")

CPMAddPackage(
    URI "gh:kynex7510/RIP#fa10b8a"
    OPTIONS "RIP_BACKEND kygx"
)

# Setup library.
add_subdirectory(Source)

if (GLASS_COMPILE_EXAMPLES)
    add_subdirectory(Examples)
endif()