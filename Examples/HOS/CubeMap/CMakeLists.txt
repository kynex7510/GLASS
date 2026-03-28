ctr_add_shader_library(CubeMap_vshader Source/Skybox.v.pica)
dkp_add_embedded_binary_library(CubeMap-shaders CubeMap_vshader)

ctr_add_graphics_target(CubeMap_skybox CUBEMAP INPUTS Gfx/Skybox.png)
dkp_add_embedded_binary_library(CubeMap-res CubeMap_skybox)

add_executable(CubeMap Source/Main.c)
target_link_libraries(CubeMap PRIVATE GLASSv2 kazmath CubeMap-shaders CubeMap-res)

ctr_create_3dsx(CubeMap)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CubeMap.3dsx TYPE BIN)