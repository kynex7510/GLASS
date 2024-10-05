#include "GLES/gl2.h"
#include <citro3d.h>

#include "Vshader_shbin.h"

typedef struct {
    float x, y, z;
} Position;

static const Position g_VertexList[3] = {
    {200.0f, 200.0f, 0.5f}, // Top
    {100.0f, 40.0f, 0.5f},  // Left
    {300.0f, 40.0f, 0.5f},  // Right
};

static GLint g_ProjLoc;
static C3D_Mtx g_Proj;

static void invertComponents(C3D_Mtx* matrix) {
    for (size_t i = 0; i < 4; ++i) {
        C3D_FVec* row = &matrix->r[i];
        float tmp = row->x;
        row->x = row->w;
        row->w = tmp;
        tmp = row->y;
        row->y = row->z;
        row->z = tmp;
    }
}

static GLuint sceneInit(void) {
    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, Vshader_shbin, Vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");

    // Create the VBO (vertex buffer object).
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes for use with the vertex shader.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Position), NULL); // v0 = position
    glEnableVertexAttribArray(0);

    glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f); // v1 = color
    glEnableVertexAttribArray(1);

    // Compute the projection matrix.
    Mtx_OrthoTilt(&g_Proj, 0.0, 400.0, 0.0, 240.0, 0.0, 1.0, true);

    // Citro3D components are inverted.
    invertComponents(&g_Proj);

    return vbo;
}

static void sceneRender(void) {
    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_Proj.m);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main() {
    // Initialize graphics.
    gfxInitDefault();

    glassCtx ctx = glassCreateContext(GLASS_VERSION_2_0);
    glassBindContext(ctx);

    // Initialize the render target.
    GLuint fb;
    GLuint rb;

    glGenFramebuffers(1, &fb);
    glGenRenderbuffers(1, &rb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 400, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);

    glViewport(0, 0, 400, 240);
    glClearColor(104 / 256.0f, 176 / 256.0f, 216 / 256.0f, 1.0f);

    // Initialize the scene.
    GLuint vbo = sceneInit();

    // Main loop.
    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu.

        // Render the scene.
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender();
        glassSwapBuffers();
        gspWaitForVBlank();
    }

    // Deinitialize graphics.
    glDeleteBuffers(1, &vbo);
    glDeleteRenderbuffers(1, &rb);
    glDeleteFramebuffers(1, &fb);

    glassDestroyContext(ctx);
    gfxExit();
    return 0;
}