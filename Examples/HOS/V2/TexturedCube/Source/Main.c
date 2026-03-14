#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "stb_image.h"
#include "TexturedCube_vshader_shbin.h"

typedef struct {
    float position[3];
    float texcoord[2];
    float normal[3];
} Vertex;

static GLint g_ProjLoc;
static GLint g_ModelViewLoc;
static GLint g_LightVecLoc;
static GLint g_LightHalfVecLoc;
static GLint g_LightClrLoc;
static GLint g_MaterialLoc;

static kmMat4 g_Projection;

static kmMat4 g_Material = {{
    0.2f, 0.2f, 0.2f, 0.0f, // Ambient
    0.4f, 0.4f, 0.4f, 0.0f, // Diffuse
    0.8f, 0.8f, 0.8f, 0.0f, // Specular
    0.0f, 0.0f, 0.0f, 1.0f, // Emission
}};

static const Vertex g_VertexList[] = {
    // First face (PZ)
    // First triangle
    { {-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f} },
    { {+0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, +1.0f} },
    { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f} },
    // Second triangle
    { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, +1.0f} },
    { {-0.5f, +0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, +1.0f} },
    { {-0.5f, -0.5f, +0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, +1.0f} },

    // Second face (MZ)
    // First triangle
    { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
    { {-0.5f, +0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
    { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
    // Second triangle
    { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
    { {+0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
    { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },

    // Third face (PX)
    // First triangle
    { {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f} },
    { {+0.5f, +0.5f, -0.5f}, {1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f} },
    { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f} },
    // Second triangle
    { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {+1.0f, 0.0f, 0.0f} },
    { {+0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {+1.0f, 0.0f, 0.0f} },
    { {+0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {+1.0f, 0.0f, 0.0f} },

    // Fourth face (MX)
    // First triangle
    { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} },
    { {-0.5f, -0.5f, +0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} },
    { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} },
    // Second triangle
    { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} },
    { {-0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f} },
    { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} },

    // Fifth face (PY)
    // First triangle
    { {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f} },
    { {-0.5f, +0.5f, +0.5f}, {1.0f, 0.0f}, {0.0f, +1.0f, 0.0f} },
    { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f} },
    // Second triangle
    { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, +1.0f, 0.0f} },
    { {+0.5f, +0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, +1.0f, 0.0f} },
    { {-0.5f, +0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, +1.0f, 0.0f} },

    // Sixth face (MY)
    // First triangle
    { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} },
    { {+0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} },
    { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
    // Second triangle
    { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
    { {-0.5f, -0.5f, +0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
    { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} },
};

#define NUM_VERTICES (sizeof(g_VertexList)/sizeof(Vertex))

static void sceneInit(u16 screenWidth, u16 screenHeight, GLuint* vbo, GLuint* tex) {
    // Set default state.
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);
    glEnable(GL_CULL_FACE);
    
    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, TexturedCube_vshader_shbin, TexturedCube_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");
    g_ModelViewLoc = glGetUniformLocation(prog, "modelView");
    g_LightVecLoc = glGetUniformLocation(prog, "lightVec");
    g_LightHalfVecLoc = glGetUniformLocation(prog, "lightHalfVec");
    g_LightClrLoc = glGetUniformLocation(prog, "lightClr");
    g_MaterialLoc = glGetUniformLocation(prog, "material");

    // Create the VBO (vertex buffer object).
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes for use with the vertex shader.
    const GLint inpos = glGetAttribLocation(prog, "inpos");
    glVertexAttribPointer(inpos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position)));
    glEnableVertexAttribArray(inpos);

    const GLint intex = glGetAttribLocation(prog, "intex");
    glVertexAttribPointer(intex, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texcoord)));
    glEnableVertexAttribArray(intex);

    const GLint innrm = glGetAttribLocation(prog, "innrm");
    glVertexAttribPointer(innrm, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(innrm);

    // Compute the projection matrix.
    kmMat4 tmp;
    kmMat4PerspTilt(&tmp, kmDegreesToRadians(80.0f), 400.0f/240.0f, 0.01f, 1000.0f, false);
    kmMat4Transpose(&g_Projection, &tmp);

    // Create texture.
    glGenTextures(1, tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Allocate texture data in VRAM.
    glTexVRAMPICA(GL_TRUE);

    // Load texture.
    stbi_set_flip_vertically_on_load(true);

    int width;
    int height;
    int channels;
    char path[80];
    for (size_t level = 0; level <= 6; ++level) {
        sprintf(path, "romfs:/Boykisser%u.png", level);
        stbi_uc* buffer = stbi_load(path, &width, &height, &channels, 3);
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
        stbi_image_free(buffer);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 6);

    // Configure the first combiner stage: modulate texture color and vertex color.
    glCombinerStagePICA(0);
    glCombinerSrcPICA(GL_SRC0_RGB, GL_TEXTURE0);
    glCombinerSrcPICA(GL_SRC0_ALPHA, GL_TEXTURE0);
    glCombinerFuncPICA(GL_COMBINE_RGB, GL_MODULATE);
    glCombinerFuncPICA(GL_COMBINE_ALPHA, GL_MODULATE);
}

static void sceneRender(float angleX, float angleY) {
    // Compute the model view matrix.
    kmMat4 tmp;
    kmMat4 tmp2;
    kmMat4 modelView;

    kmMat4Identity(&tmp);
    kmMat4Translation(&tmp2, 0.0f, 0.0f, -2.0f + 0.5f * sinf(angleX));
    kmMat4Multiply(&modelView, &tmp, &tmp2);

    memcpy(&tmp, &modelView, sizeof(kmMat4));
    kmMat4RotationX(&tmp2, angleX);
    kmMat4Multiply(&modelView, &tmp, &tmp2);

    kmMat4RotationY(&tmp2, angleY);
    kmMat4Multiply(&tmp, &modelView, &tmp2);
    kmMat4Transpose(&modelView, &tmp);

    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_Projection.mat);
    glUniformMatrix4fv(g_ModelViewLoc, 1, GL_FALSE, modelView.mat);
    glUniformMatrix4fv(g_MaterialLoc, 1, GL_FALSE, g_Material.mat);
    glUniform4f(g_LightVecLoc, 0.0f, 0.0f, -1.0f, 0.0f);
    glUniform4f(g_LightHalfVecLoc, 0.0f, 0.0f, -1.0f, 0.0f);
    glUniform4f(g_LightClrLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES);
}

int main() {
    romfsInit();

    // Initialize graphics.
    gfxInitDefault();
    kygxInit();

    GLASSCtx ctx = glassCreateDefaultContext(GLASS_VERSION_ES_2);
    glassBindContext(ctx);

    // Initialize the render target.
    u16 screenWidth = 0;
    u16 screenHeight = 0;
    glassGetScreenFramebuffer(ctx, &screenWidth, &screenHeight, NULL);

    GLuint fb;
    GLuint rb;

    glGenFramebuffers(1, &fb);
    glGenRenderbuffers(1, &rb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);

    // Initialize the scene.
    GLuint vbo;
    GLuint tex;
    sceneInit(screenWidth, screenHeight, &vbo, &tex);

    // Main loop.
    float angleX = 0.0f, angleY = 0.0f;

    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu.

        // Rotate the cube each frame
        angleX += kmPI / 180;
        angleY += kmPI / 360;

        // Render the scene.
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender(angleX, angleY);
        glassSwapBuffers();
    }

    // Deinitialize graphics.
    glDeleteTextures(1, &tex);
    glDeleteBuffers(1, &vbo);
    glDeleteRenderbuffers(1, &rb);
    glDeleteFramebuffers(1, &fb);

    glassDestroyContext(ctx);

    kygxExit();
    gfxExit();
    romfsExit();
    return 0;
}