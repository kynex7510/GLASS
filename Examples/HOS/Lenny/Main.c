#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "Lenny.h"
#include "Lenny_vshader_shbin.h"

static GLint g_ProjLoc;
static GLint g_ModelViewLoc;

static kmMat4 g_BaseModelView;

static GLuint sceneInit(void) {
    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, Lenny_vshader_shbin, Lenny_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

     // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");
    g_ModelViewLoc = glGetUniformLocation(prog, "modelView");

    // Create and bind the VBO (vertex buffer object).
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x)); // v0 = position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx)); // v1 = normal
    glEnableVertexAttribArray(1);

    // Compute the base model view matrix (avoid computation in the rendering step).
    kmVec4 objPos;
    kmVec4Fill(&objPos, 0.0f, 0.0f, -3.0f, 1.0f);

    kmMat4 tmp;
    kmMat4 tmp2;

    kmMat4Identity(&tmp);
    kmMat4Translation(&tmp2, objPos.x, objPos.y, objPos.z);
    kmMat4Multiply(&g_BaseModelView, &tmp, &tmp2);

    memcpy(&tmp, &g_BaseModelView, sizeof(kmMat4));
    kmMat4Scaling(&tmp2, 2.0f, 2.0f, 2.0f);
    kmMat4Multiply(&g_BaseModelView, &tmp, &tmp2);
}

static void sceneRender(float iod, float angleX, float angleY) {
    // Compute the projection matrix.
    kmMat4 tmp;
    kmMat4 projection;
    kmMat4PerspStereoTilt(&tmp, kmDegreesToRadians(40.0f), 400.0f/240.0f, 0.01f, 1000.0f, iod, 2.0f, false);
    kmMat4Transpose(&projection, &tmp);

    // Compute the model matrix.
    kmMat4 modelView;
    kmMat4RotationY(&modelView, angleY * kmPI * 2);
    kmMat4Multiply(&tmp, &g_BaseModelView, &modelView);
    kmMat4Transpose(&modelView, &tmp);

    // Update the uniforms.
	glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, projection.mat);
	glUniformMatrix4fv(g_ModelViewLoc, 1, GL_FALSE, modelView.mat);

    // Draw the VBO.
	glDrawArrays(GL_TRIANGLES, 0, VERTEX_LIST_COUNT);
}

static void createFramebuffer(GLuint* fb, GLuint* rb) {
    glGenFramebuffers(1, fb);
    glGenRenderbuffers(2, rb);

    glBindFramebuffer(GL_FRAMEBUFFER, *fb);

    glBindRenderbuffer(GL_RENDERBUFFER, rb[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, 400, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb[0]);

    glBindRenderbuffer(GL_RENDERBUFFER, rb[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, 400, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb[1]);
}

int main() {
    // Initialize graphics.
    gfxInitDefault();
    kygxInit();

    // Enable stereoscopic 3D.
    gfxSet3D(true);

    // Create context.
    GLASSCtx ctx = glassCreateDefaultContext(GLASS_VERSION_2_0);
    glassBindContext(ctx);

    /// Initialize the render targets.
    GLuint leftFB;
    GLuint leftRBs[2];

    glassSetTargetSide(ctx, GLASS_SIDE_LEFT);
    createFramebuffer(&leftFB, leftRBs);

    GLuint rightFB;
    GLuint rightRBs[2];

    glassSetTargetSide(ctx, GLASS_SIDE_RIGHT);
    createFramebuffer(&rightFB, rightRBs);

    // Set default state.
    glViewport(0, 0, 400, 240);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);
    glClearDepthf(0.0f);

    // Initialize the scene.
    GLuint vbo = sceneInit();

    // Main loop.
	float angleX = 0.0;
	float angleY = 0.0;

    bool doneDraw = false;
    while ( aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu.

        const float slider = osGet3DSliderState();
        const float iod = slider / 3;

        // Rotate the model
        if (!(kHeld & KEY_A)) {
            angleX += 1.0f/64;
            angleY += 1.0f/256;
        }

		// Render left.
        glassSetTargetSide(ctx, GLASS_SIDE_LEFT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sceneRender(-iod, angleX, angleY);

        // Render right.
        if (iod > 0.0f) {
            glassSetTargetSide(ctx, GLASS_SIDE_RIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            sceneRender(iod, angleX, angleY);
        }

        glassSwapBuffers();
    }

	// Deinitialize graphics.
    glDeleteBuffers(1, &vbo);

    glDeleteRenderbuffers(2, rightRBs);
    glDeleteFramebuffers(1, &rightFB);

	glDeleteRenderbuffers(2, leftRBs);
	glDeleteFramebuffers(1, &rightFB);

    glassDestroyContext(ctx);

    kygxExit();
    gfxExit();
    return 0;
}