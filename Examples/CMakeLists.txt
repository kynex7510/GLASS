CPMAddPackage(
    URI "gh:kynex7510/kazmath_3ds#e3f3d3b"
    OPTIONS "KAZMATH_BUILD_GL_UTILS OFF"
)

if (CTR_BAREMETAL)
    add_subdirectory(BM)
else()
    add_subdirectory(HOS)
endif()