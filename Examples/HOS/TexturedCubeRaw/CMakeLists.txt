ctr_add_shader_library(TexturedCubeRaw_vshader Source/VShader.v.pica)
dkp_add_embedded_binary_library(TexturedCubeRaw-shaders TexturedCubeRaw_vshader)

add_executable(TexturedCubeRaw Source/Main.c Source/stb_image.c)
target_link_libraries(TexturedCubeRaw PRIVATE GLASSv2 kazmath TexturedCubeRaw-shaders)

ctr_create_3dsx(TexturedCubeRaw ROMFS RomFS)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TexturedCubeRaw.3dsx TYPE BIN)