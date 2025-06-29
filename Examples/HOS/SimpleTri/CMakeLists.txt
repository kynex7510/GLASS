ctr_add_shader_library(vshader VShader.v.pica)
dkp_add_embedded_binary_library(shaders vshader)

add_executable(SimpleTri Main.c)
target_link_libraries(SimpleTri PRIVATE GLASSv2 shaders)

ctr_create_3dsx(SimpleTri)