set(GLASS_SOURCES
    ${PROJECT_SOURCE_DIR}/Source/Base/Context.c
    ${PROJECT_SOURCE_DIR}/Source/Base/GLASS.c
    ${PROJECT_SOURCE_DIR}/Source/Base/Math.c
    ${PROJECT_SOURCE_DIR}/Source/Base/MathCTRU.c
    ${PROJECT_SOURCE_DIR}/Source/Base/Memory.c
    ${PROJECT_SOURCE_DIR}/Source/Base/Read.c
    ${PROJECT_SOURCE_DIR}/Source/Base/TexManager.c
    ${PROJECT_SOURCE_DIR}/Source/Base/Types.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Attribs.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Buffers.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Combiners.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Effects.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Fog.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Framebuffer.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Get.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Rendering.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Shaders.c
    ${PROJECT_SOURCE_DIR}/Source/Common/State.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Texture.c
    ${PROJECT_SOURCE_DIR}/Source/Common/Uniforms.c
    ${PROJECT_SOURCE_DIR}/Source/Platform/GFX.c
    ${PROJECT_SOURCE_DIR}/Source/Platform/GPU.c
)

add_library(GLASSv2 STATIC ${GLASS_SOURCES})

target_include_directories(GLASSv2 PUBLIC ${PROJECT_SOURCE_DIR}/Include PRIVATE ${PROJECT_SOURCE_DIR}/Source)
target_link_libraries(GLASSv2 PUBLIC kygx rip)
target_compile_options(GLASSv2 PRIVATE -Wall -Werror)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(GLASSv2 PRIVATE GLASS_NO_MERCY)
endif()

install(TARGETS GLASSv2)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/Include DESTINATION include)