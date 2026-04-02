ctr_add_shader_library(MipMapFog_vshader Source/VShader.v.pica)
dkp_add_embedded_binary_library(MipMapFog-shaders MipMapFog_vshader)

ctr_add_graphics_target(MipMapFog_kitten TEXTURE INPUTS Gfx/Kitten.png)
dkp_add_embedded_binary_library(MipMapFog-gfx MipMapFog_kitten)

add_executable(MipMapFog Source/Main.c)
target_link_libraries(MipMapFog PRIVATE GLASSv2 kazmath MipMapFog-shaders MipMapFog-gfx)

ctr_create_3dsx(MipMapFog)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MipMapFog.3dsx TYPE BIN)