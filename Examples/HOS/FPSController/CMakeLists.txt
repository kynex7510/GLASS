ctr_add_shader_library(FPSController_vshader VShader.v.pica)
dkp_add_embedded_binary_library(FPSController-shaders FPSController_vshader)

add_executable(FPSController Main.c)
target_link_libraries(FPSController PRIVATE GLASSv2 kazmath FPSController-shaders)

ctr_create_3dsx(FPSController)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FPSController.3dsx TYPE BIN)