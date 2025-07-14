#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "BothScreens_vshader_shbin.h"

typedef struct {
    float x, y, z;
} Vertex;

static const Vertex g_VertexList[3] = {
    {    0.0f, 200.0f, 0.5f },
    { -100.0f,  40.0f, 0.5f },
    {  100.0f,  40.0f, 0.5f },
};

#define VERTEX_LIST_COUNT (sizeof(g_VertexList)/sizeof(g_VertexList[0]))

static GLint g_TopProjLoc;
static GLint g_BottomProjLoc;

static kmMat4 g_ProjectionTop;
static kmMat4 g_ProjectionBottom;

static bool isTopScreen(void) { return glassGetTargetScreen(glassGetBoundContext()) == GLASS_SCREEN_TOP; }

static void sceneInit(GLuint* vbo) {
    const bool isTop = isTopScreen();

    // Set default state.
    glViewport(0, 0, isTop ? 400 : 320, 240);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);

    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, BothScreens_vshader_shbin, BothScreens_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Get the location of the uniforms.
    if (isTop) {
        g_TopProjLoc = glGetUniformLocation(prog, "projection");
    } else {
        g_BottomProjLoc = glGetUniformLocation(prog, "projection");
    }

    // Create the VBO (vertex buffer object).
    glGenBuffers(1, vbo);

    // Configure vertex attribute.
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL); // v0 = position
    glEnableVertexAttribArray(0);

    glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f); // v1 = color
    glEnableVertexAttribArray(1);
}

static void sceneRender(float a) {
    const bool isTop = isTopScreen();

    // Compute color.
	float b = (cosf(a * kmPI * 2) + 1.0f) / 2.0f;
	if (!isTop)
        b = 1.0f - b;

    // Update the uniforms.
    glUniformMatrix4fv(isTop ? g_TopProjLoc : g_BottomProjLoc, 1, GL_FALSE, isTop ? g_ProjectionTop.mat : g_ProjectionBottom.mat);
    glVertexAttrib3f(1, b, b, b);

	// Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, VERTEX_LIST_COUNT);
}

static void createFramebuffer(GLuint* fb, GLuint* rb) {
    const bool isTop = isTopScreen();

    glGenFramebuffers(1, fb);
    glGenRenderbuffers(1, rb);

    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glBindRenderbuffer(GL_RENDERBUFFER, *rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, isTop ? 400 : 320, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *rb);
}

int main() {
    // Initialize graphics.
    gfxInitDefault();
    kygxInit();

    // Create contexts.
    GLASSCtxParams ctxParams;
    glassGetDefaultContextParams(&ctxParams, GLASS_VERSION_2_0);

    GLASSCtx topCtx = glassCreateContext(&ctxParams);

    ctxParams.targetScreen = GLASS_SCREEN_BOTTOM;
    GLASSCtx bottomCtx = glassCreateContext(&ctxParams);

    // Initialize the render targets.
    GLuint topFB;
    GLuint topRB;
    GLuint bottomFB;
    GLuint bottomRB;

    glassBindContext(topCtx);
    createFramebuffer(&topFB, &topRB);
    
    glassBindContext(bottomCtx);
    createFramebuffer(&bottomFB, &bottomRB);

    // Initialize the scene.
    GLuint topVBO;
    glassBindContext(topCtx);
    sceneInit(&topVBO);

    GLuint bottomVBO;
    glassBindContext(bottomCtx);
    sceneInit(&bottomVBO);

    // Compute the projection matrices.
    kmMat4 tmp;
    kmMat4OrthoTilt(&tmp, -200.0f, 200.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);
    kmMat4Transpose(&g_ProjectionTop, &tmp);

    kmMat4OrthoTilt(&tmp, -160.0f, 160.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);
    kmMat4Transpose(&g_ProjectionBottom, &tmp);

    // Main loop.
    float count = 0.0f;
    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu.

        // Render top.
        glassBindContext(topCtx);
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender(count);

        // Render bottom.
        glassBindContext(bottomCtx);
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender(count);

        // Swap both buffers.
        glassSwapContextBuffers(topCtx, bottomCtx);
        count += 1/128.0f;
    }

    // Deinitialize graphics.
    glassBindContext(topCtx);
    glDeleteBuffers(1, &topVBO);
    glDeleteBuffers(1, &topRB);
    glDeleteBuffers(1, &topFB);

    glassBindContext(bottomCtx);
    glDeleteBuffers(1, &bottomVBO);
    glDeleteBuffers(1, &bottomRB);
    glDeleteBuffers(1, &bottomFB);

    glassDestroyContext(topCtx);
    glassDestroyContext(bottomCtx);

    kygxExit();
    gfxExit();
    return 0;
}