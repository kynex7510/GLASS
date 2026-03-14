ctr_add_shader_library(GeoShader_shaders Program.v.pica Program.g.pica)
dkp_add_embedded_binary_library(GeoShader-shaders GeoShader_shaders)

add_executable(GeoShader Main.c)
target_link_libraries(GeoShader PRIVATE GLASSv2 kazmath GeoShader-shaders)

ctr_create_3dsx(GeoShader)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GeoShader.3dsx TYPE BIN)