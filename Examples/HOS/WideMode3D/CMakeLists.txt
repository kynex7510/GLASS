ctr_add_shader_library(WideMode3D_vshader VShader.v.pica)
dkp_add_embedded_binary_library(WideMode3D-shaders WideMode3D_vshader)

add_executable(WideMode3D Main.c Lenny.c)
target_link_libraries(WideMode3D PRIVATE GLASSv2 kazmath WideMode3D-shaders)

ctr_create_3dsx(WideMode3D)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WideMode3D.3dsx TYPE BIN)