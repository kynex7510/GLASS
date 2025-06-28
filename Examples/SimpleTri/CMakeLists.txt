cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
project(SimpleTri C ASM)

ctr_add_shader_library(vshader VShader.v.pica)
dkp_add_embedded_binary_library(shaders vshader)

add_executable(SimpleTri Main.c)
target_link_libraries(SimpleTri PRIVATE GLASSv2 shaders)

ctr_create_3dsx(SimpleTri)