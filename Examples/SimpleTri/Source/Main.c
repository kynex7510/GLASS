#include "GLES/gl2.h"

#include "Vshader_shbin.h"

typedef struct {
    float x, y;
} Vertex;

// Both screens are rotated by 90 degrees, clockwise.
// Bottom left is (-1.0, 1.0), top right is (1.0, -1.0).
static const Vertex g_VertexList[3] = {
    { 0.66f, 0.0f }, // Top
    { -0.66f, -0.5f }, // Left
    { -0.66f, 0.5f }, // Right
};

static GLuint sceneInit() {
    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, Vshader_shbin, Vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Create the VBO (vertex buffer object).
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes for use with the vertex shader.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL); // v0 = position
    glEnableVertexAttribArray(0);

    glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f); // v1 = color
    glEnableVertexAttribArray(1);

    return vbo;
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
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, 400, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);

    glViewport(0, 0, 400, 240);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);

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
        glDrawArrays(GL_TRIANGLES, 0, 3);
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