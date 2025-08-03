include(BM/ARM11)

ctr_add_shader_library(vshader VShader.v.pica)
dkp_add_embedded_binary_library(shaders vshader)

add_executable(SimpleTri Main.c)
target_link_libraries(SimpleTri PRIVATE GLASS shaders)
bm_target_linker_file(SimpleTri ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default11.ld)

add_firm_binary(SimpleTri_bin SimpleTri)