#include <GLES/gl2.h>
#include <citro3d.h>

#include "Vshader_shbin.h"
#include "Teapot.h"

static GLint g_ProjLoc;
static GLint g_ModelViewLoc;

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

static void sceneInit(GLuint* buffers) {
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
    g_ModelViewLoc = glGetUniformLocation(prog, "modelView");

    // Create the VBO (vertex buffer object) and IBO (index buffer object).
    glGenBuffers(2, buffers);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_VertexElements), g_VertexElements, GL_STATIC_DRAW);

    // Configure attributes for use with the vertex shader.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float[6]), NULL); // v0 = position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float[6]), (void*)(sizeof(float[6]) / 2)); // v1 = normal
    glEnableVertexAttribArray(1);
}

static void sceneRender(float iod, float angleX, float angleY) {
    // Compute the projection matrix.
    C3D_Mtx proj;
    Mtx_PerspStereoTilt(&proj, C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, iod, 2.0f, false);

    C3D_FVec objPos = FVec4_New(0.0f, 0.0f, -3.0f, 1.0f);

    // Calculate the modelView matrix
    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView, objPos.x, objPos.y, objPos.z, true);
    Mtx_RotateX(&modelView, C3D_Angle(sinf(angleX)/4), true);
    Mtx_RotateY(&modelView, C3D_Angle(angleY), true);
    Mtx_Scale(&modelView, 2.0f, 2.0f, 2.0f);

    // Update the uniforms.
    invertComponents(&proj);
    invertComponents(&modelView);

    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, proj.m);
    glUniformMatrix4fv(g_ModelViewLoc, 1, GL_FALSE, modelView.m);

    // Draw the VBO.
    glDrawElements(GL_TRIANGLES, NUM_ELEMENTS, GL_UNSIGNED_SHORT, NULL);
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

    // Initialize the scene.
    GLuint buffers[2];
    sceneInit(buffers);

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

        float slider = osGet3DSliderState();
        float iod = slider/3;

        // Rotate the model
        if (!(kHeld & KEY_A)) {
            angleX += 1.0f/64;
            angleY += 1.0f/256;
        }

        // Render the scene.
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender(-iod, angleX, angleY);
        glassSwapBuffers();
        gspWaitForVBlank();
    }

    // Deinitialize graphics.
    glDeleteBuffers(2, buffers);
    glDeleteRenderbuffers(1, &rb);
    glDeleteFramebuffers(1, &fb);

    glassDestroyContext(ctx);
    gfxExit();
    return 0;
}