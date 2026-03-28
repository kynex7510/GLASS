ctr_add_shader_library(SimpleTri_vshader VShader.v.pica)
dkp_add_embedded_binary_library(SimpleTri-shaders SimpleTri_vshader)

add_executable(SimpleTri Main.c)
target_link_libraries(SimpleTri PRIVATE GLASSv2 SimpleTri-shaders)

ctr_create_3dsx(SimpleTri)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SimpleTri.3dsx TYPE BIN)