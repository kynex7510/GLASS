ctr_add_shader_library(BothScreens_vshader VShader.v.pica)
dkp_add_embedded_binary_library(BothScreens-shaders BothScreens_vshader)

add_executable(BothScreens Main.c)
target_link_libraries(BothScreens PRIVATE GLASSv2 kazmath BothScreens-shaders)

ctr_create_3dsx(BothScreens)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BothScreens.3dsx TYPE BIN)