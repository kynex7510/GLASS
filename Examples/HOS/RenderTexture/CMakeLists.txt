ctr_add_shader_library(RenderTexture_vshader VShader.v.pica)
dkp_add_embedded_binary_library(RenderTexture-shaders RenderTexture_vshader)

add_executable(RenderTexture Main.c)
target_link_libraries(RenderTexture PRIVATE GLASSv2 kazmath RenderTexture-shaders)

ctr_create_3dsx(RenderTexture)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RenderTexture.3dsx TYPE BIN)