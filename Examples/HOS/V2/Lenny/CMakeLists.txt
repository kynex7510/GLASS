ctr_add_shader_library(Lenny_vshader VShader.v.pica)
dkp_add_embedded_binary_library(Lenny-shaders Lenny_vshader)

add_executable(Lenny Main.c Lenny.c)
target_link_libraries(Lenny PRIVATE GLASS kazmath Lenny-shaders)

ctr_create_3dsx(Lenny)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Lenny.3dsx TYPE BIN)