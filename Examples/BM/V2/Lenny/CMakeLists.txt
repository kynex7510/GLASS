include(BM/ARM11)

ctr_add_shader_library(Lenny_vshader VShader.v.pica)
dkp_add_embedded_binary_library(Lenny-shaders Lenny_vshader)

add_executable(Lenny Main.c Lenny.c)
target_link_libraries(Lenny PRIVATE GLASS kazmath m Lenny-shaders)
bm_target_linker_file(Lenny ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)

add_firm_binary(Lenny_bin Lenny)