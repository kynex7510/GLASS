CPMAddPackage("gh:kynex7510/kazmath_3ds#d978c36")

if (CTR_BAREMETAL)
    add_subdirectory(BM)
else()
    add_subdirectory(HOS)
endif()