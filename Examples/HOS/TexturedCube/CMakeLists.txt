ctr_add_shader_library(TexturedCube_vshader Source/VShader.v.pica)
dkp_add_embedded_binary_library(TexturedCube-shaders TexturedCube_vshader)

add_executable(TexturedCube Source/Main.c Source/stb_image.c)
target_link_libraries(TexturedCube PRIVATE GLASSv2 kazmath TexturedCube-shaders)

ctr_create_3dsx(TexturedCube ROMFS RomFS)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TexturedCube.3dsx TYPE BIN)