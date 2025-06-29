include(BM/N3DS9)

add_executable(Common9 Main.c)
target_link_libraries(Common9 PRIVATE n3ds9)
bm_target_linker_file(Common9 ${CTR_BM_TOOLCHAIN_ROOT}/Linker/Default9.ld)
add_firm_binary(Common9 Common9_bin)