#include <GLES/gl2.h>
#include <citro3d.h>

#include "Skybox_shbin.h"
#include "Skybox_t3x.h"

typedef struct {
    float position[3];
} Vertex;

static GLint g_ProjLoc;
static GLint g_ModelViewLoc;

static C3D_Mtx g_Proj;

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

static void sceneInit(GLuint* vbo, GLuint* tex) {
    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, Skybox_shbin, Skybox_shbin_size);
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
    Mtx_PerspTilt(&g_Proj, C3D_AngleFromDegrees(45.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

    // Citro3D components are inverted.
    invertComponents(&g_Proj);

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
    glassTexture skyboxTex;
    glassLoadTexture(Skybox_t3x, Skybox_t3x_size, &skyboxTex);
    glassMoveTextureData(&skyboxTex);
    glassDestroyTexture(&skyboxTex);

    // Configure the first combiner stage: use the texture color.
    glCombinerStagePICA(0);
    glCombinerSrcPICA(GL_SRC0_RGB, GL_TEXTURE0);
    glCombinerSrcPICA(GL_SRC0_ALPHA, GL_TEXTURE0);
    glCombinerFuncPICA(GL_COMBINE_RGB, GL_REPLACE);
    glCombinerFuncPICA(GL_COMBINE_ALPHA, GL_REPLACE);
}

static void sceneRender(float angleX, float angleY) {
    // Calculate the modelView matrix.
    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_RotateX(&modelView, angleX, true);
    Mtx_RotateY(&modelView, angleY, true);
    invertComponents(&modelView);

    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_Proj.m);
    glUniformMatrix4fv(g_ModelViewLoc, 1, GL_FALSE, modelView.m);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES);
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
    glClearColor(104 / 256.0f, 176 / 256.0f, 216 / 256.0f, 1.0f);

    // Initialize the scene
    GLuint vbo;
    GLuint tex;
    sceneInit(&vbo, &tex);

    // Main loop
    float angleX = 0.0f, angleY = 0.0f;

    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu

        if (kHeld & KEY_UP) {
            angleX -= C3D_AngleFromDegrees(2.0f);
            if (angleX < C3D_AngleFromDegrees(-90.0f))
                angleX = C3D_AngleFromDegrees(-90.0f);
        } else if (kHeld & KEY_DOWN) {
            angleX += C3D_AngleFromDegrees(2.0f);
            if (angleX > C3D_AngleFromDegrees(90.0f))
                angleX = C3D_AngleFromDegrees(90.0f);
        }

        if (kHeld & KEY_LEFT) {
            angleY -= C3D_AngleFromDegrees(2.0f);
        } else if (kHeld & KEY_RIGHT) {
            angleY += C3D_AngleFromDegrees(2.0f);
        }

        // Render the scene.
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender(angleX, angleY);
        glassSwapBuffers();
        gspWaitForVBlank();
    }

    // Deinitialize graphics.
    glDeleteTextures(1, &tex);
    glDeleteBuffers(1, &vbo);
    glDeleteRenderbuffers(1, &rb);
    glDeleteFramebuffers(1, &fb);

    glassDestroyContext(ctx);
    gfxExit();
    return 0;
}
