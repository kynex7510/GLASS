#ifndef _GLASS_GL_H
#define _GLASS_GL_H

#include <GLASS.h>
#include <GLASS/CommonAPI.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define glTexEnvStagePICA glCombinerStagePICA

/* ClipPlane */

void glClipPlanef(GLenum p, const GLfloat* eqn);
void glGetClipPlanef(GLenum plane, GLfloat* equation);

/* Color */

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);

/* Fog */

void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat* params);

/* Light */

void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightfv(GLenum light, GLenum pname, const GLfloat* params);
void glLightModelf(GLenum pname, GLfloat param);
void glLightModelfv(GLenum pname, const GLfloat* params);
void glGetLightfv(GLenum light, GLenum pname, GLfloat* params);

/* Material */

void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat* params);
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat* params);

/* Matrix */

void glPushMatrix(void);
void glPopMatrix(void);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat* m);
void glMatrixMode(GLenum mode);
void glMultMatrixf(const GLfloat* m);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
void glFrustumf(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);

/* Normal */

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glNormalPointer(GLenum type, GLsizei stride, const void* pointer);

/* Point */

void glPointParameterf(GLenum pname, GLfloat param);
void glPointParameterfv(GLenum pname, const GLfloat* params);
void glPointSize(GLfloat size);

/* State */

void glClientActiveTexture(GLenum texture);
void glDisableClientState(GLenum array);
void glEnableClientState(GLenum array);
void glLineWidth(GLfloat width);
void glHint(GLenum target, GLenum mode);
void glPixelStorei(GLenum pname, GLint param);

/* TexCoord */

void glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);

/* TexEnv (wrapper to combiners?) */

void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params);
void glTexEnviv(GLenum target, GLenum pname, const GLint* params);
void glGetTexEnviv(GLenum target, GLenum pname, GLint* params);
void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat* params);

/* Vertex */

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);

// ...

void glGetPointerv (GLenum pname, void** params);
void glSampleCoverage (GLfloat value, GLboolean invert);
void glShadeModel (GLenum mode);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _GLASS_GL_H */