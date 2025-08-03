ctr_add_shader_library(MultipleBuf_vshader VShader.v.pica)
dkp_add_embedded_binary_library(MultipleBuf-shaders MultipleBuf_vshader)

add_executable(MultipleBuf Main.c)
target_link_libraries(MultipleBuf PRIVATE GLASS kazmath MultipleBuf-shaders)

ctr_create_3dsx(MultipleBuf)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MultipleBuf.3dsx TYPE BIN)