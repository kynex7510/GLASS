#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "CubeMap_vshader_shbin.h"
#include "CubeMap_skybox_t3x.h"

typedef struct {
    float position[3];
} Vertex;

static GLint g_ProjLoc;
static GLint g_ModelViewLoc;

static  kmMat4 g_Projection;

static const Vertex g_VertexList[] = {
    // First face (PZ)
    // First triangle
    { {-0.5f, -0.5f, +0.5f} },
    { {+0.5f, -0.5f, +0.5f} },
    { {+0.5f, +0.5f, +0.5f} },
    // Second triangle
    { {+0.5f, +0.5f, +0.5f} },
    { {-0.5f, +0.5f, +0.5f} },
    { {-0.5f, -0.5f, +0.5f} },

    // Second face (MZ)
    // First triangle
    { {-0.5f, -0.5f, -0.5f} },
    { {-0.5f, +0.5f, -0.5f} },
    { {+0.5f, +0.5f, -0.5f} },
    // Second triangle
    { {+0.5f, +0.5f, -0.5f} },
    { {+0.5f, -0.5f, -0.5f} },
    { {-0.5f, -0.5f, -0.5f} },

    // Third face (PX)
    // First triangle
    { {+0.5f, -0.5f, -0.5f} },
    { {+0.5f, +0.5f, -0.5f} },
    { {+0.5f, +0.5f, +0.5f} },
    // Second triangle
    { {+0.5f, +0.5f, +0.5f} },
    { {+0.5f, -0.5f, +0.5f} },
    { {+0.5f, -0.5f, -0.5f} },

    // Fourth face (MX)
    // First triangle
    { {-0.5f, -0.5f, -0.5f} },
    { {-0.5f, -0.5f, +0.5f} },
    { {-0.5f, +0.5f, +0.5f} },
    // Second triangle
    { {-0.5f, +0.5f, +0.5f} },
    { {-0.5f, +0.5f, -0.5f} },
    { {-0.5f, -0.5f, -0.5f} },

    // Fifth face (PY)
    // First triangle
    { {-0.5f, +0.5f, -0.5f} },
    { {-0.5f, +0.5f, +0.5f} },
    { {+0.5f, +0.5f, +0.5f} },
    // Second triangle
    { {+0.5f, +0.5f, +0.5f} },
    { {+0.5f, +0.5f, -0.5f} },
    { {-0.5f, +0.5f, -0.5f} },

    // Sixth face (MY)
    // First triangle
    { {-0.5f, -0.5f, -0.5f} },
    { {+0.5f, -0.5f, -0.5f} },
    { {+0.5f, -0.5f, +0.5f} },
    // Second triangle
    { {+0.5f, -0.5f, +0.5f} },
    { {-0.5f, -0.5f, +0.5f} },
    { {-0.5f, -0.5f, -0.5f} },
};

#define NUM_VERTICES (sizeof(g_VertexList)/sizeof(Vertex))

static void sceneInit(GLuint* vbo, GLuint* tex) {
    // Set default state.
    glViewport(0, 0, 400, 240);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);

    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, CubeMap_vshader_shbin, CubeMap_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");
    g_ModelViewLoc = glGetUniformLocation(prog, "modelView");

    // Create the VBO (vertex buffer object).
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes for use with the vertex shader.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position))); // // v0 = position
    glEnableVertexAttribArray(0);

    // Compute the projection matrix.
    kmMat4 tmp;
    kmMat4PerspTilt(&tmp, kmDegreesToRadians(45.0f), 400.0f/240.0f, 0.01f, 1000.0f, false);
    kmMat4Transpose(&g_Projection, &tmp);

    // Create texture.
    glGenTextures(1, tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *tex);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Allocate texture data in VRAM.
    glTexVRAMPICA(GL_TRUE);

    // Load texture.
    RIPTex3DS skyboxTex;
    ripLoadTex3DS(CubeMap_skybox_t3x, CubeMap_skybox_t3x_size, &skyboxTex);
    glassMoveTex3DS(&skyboxTex);
    ripDestroyTex3DS(&skyboxTex);

    // Configure the first combiner stage: use the texture color.
    glCombinerStagePICA(0);
    glCombinerSrcPICA(GL_SRC0_RGB, GL_TEXTURE0);
    glCombinerSrcPICA(GL_SRC0_ALPHA, GL_TEXTURE0);
    glCombinerFuncPICA(GL_COMBINE_RGB, GL_REPLACE);
    glCombinerFuncPICA(GL_COMBINE_ALPHA, GL_REPLACE);
}

static void sceneRender(float angleX, float angleY) {
    // Calculate the modelView matrix.
    kmMat4 modelView;
    kmMat4 tmp;
    kmMat4 tmp2;

    kmMat4Identity(&tmp);
    kmMat4RotationX(&tmp2, kmDegreesToRadians(angleX));
    kmMat4Multiply(&modelView, &tmp, &tmp2);

    kmMat4RotationY(&tmp2, kmDegreesToRadians(angleY));
    kmMat4Multiply(&tmp, &modelView, &tmp2);
    kmMat4Transpose(&modelView, &tmp);

    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_Projection.mat);
    glUniformMatrix4fv(g_ModelViewLoc, 1, GL_FALSE, modelView.mat);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES);
}

int main() {
    // Initialize graphics.
    gfxInitDefault();
    kygxInit();

    // Create context.
    GLASSCtx ctx = glassCreateDefaultContext(GLASS_VERSION_ES_2);
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

    // Initialize the scene.
    GLuint vbo;
    GLuint tex;
    sceneInit(&vbo, &tex);

    // Main loop.
    float angleX = 0.0f;
    float angleY = 0.0f;

    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu.

        if (kHeld & KEY_UP) {
            angleX -= 2.0f;
            if (angleX < -90.0f)
                angleX = -90.0f;
        } else if (kHeld & KEY_DOWN) {
            angleX += 2.0f;
            if (angleX > 90.0f)
                angleX = 90.0f;
        }

        if (kHeld & KEY_LEFT) {
            angleY -= 2.0f;
        } else if (kHeld & KEY_RIGHT) {
            angleY += 2.0f;
        }

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
    return 0;
}