# API coverage

## Common

### Buffers

| Name                   | Available? |
|------------------------|------------|
| glBindBuffer           | Yes        |
| glBufferData           | Yes        |
| glBufferSubData        | Yes        |
| glDeleteBuffers        | Yes        |
| glGenBuffers           | Yes        |
| glGetBufferParameteriv | Yes        |
| glIsBuffer             | Yes        |

### Combiners (extensions)

| Name                | Available? |
|---------------------|------------|
| glCombinerColorPICA | Yes        |
| glCombinerFuncPICA  | Yes        |
| glCombinerOpPICA    | Yes        |
| glCombinerScalePICA | Yes        |
| glCombinerSrcPICA   | Yes        |
| glCombinerStagePICA | Yes        |

### Effects

| Name                    | Available? |
|-------------------------|------------|
| glAlphaFunc             | Yes        |
| glBlendColor            | Yes        |
| glBlendEquation         | Yes        |
| glBlendEquationSeparate | Yes        |
| glBlendFunc             | Yes        |
| glBlendFuncSeparate     | Yes        |
| glColorMask             | Yes        |
| glCullFace              | Yes        |
| glDepthFunc             | Yes        |
| glDepthMask             | Yes        |
| glDepthRangef           | Yes        |
| glFrontFace             | Yes        |
| glLogicOp               | Yes        |
| glPolygonOffset         | Yes        |
| glScissor               | Yes        |
| glStencilFunc           | Yes        |
| glStencilMask           | Yes        |
| glStencilOp             | Yes        |
| glViewport              | Yes        |

### Rendering

| Name           | Available? |
| -------------- | ---------- |
| glClear        | Yes        |
| glClearColor   | Yes        |
| glClearDepthf  | Yes        |
| glClearStencil | Yes        |
| glDrawArrays   | Yes        |
| glDrawElements | Yes        |
| glFinish       | Yes        |
| glFlush        | Yes        |
| glReadPixels   | No         |

### State

| Name          | Available? |
| ------------- | ---------- |
| glDisable     | Yes        |
| glEnable      | Yes        |
| glIsEnabled   | Yes        |
| glGetError    | Yes        |
| glGetBooleanv | No         |
| glGetFloatv   | No         |
| glGetIntegerv | No         |
| glGetString   | Yes        |

### Texture

| Name                      | Available? |
| ------------------------- | ---------- |
| glActiveTexture           | Yes        |
| glBindTexture             | Yes        |
| glCompressedTexImage2D    | Stubbed    |
| glCompressedTexSubImage2D | Stubbed    |
| glCopyTexImage2D          | No         |
| glCopyTexSubImage2D       | No         |
| glDeleteTextures          | Yes        |
| glGenTextures             | Yes        |
| glGetTexParameterfv       | No         |
| glGetTexParameteriv       | No         |
| glIsTexture               | Yes        |
| glTexImage2D              | Yes        |
| glTexParameterf           | Yes        |
| glTexParameteri           | Yes        |
| glTexParameterfv          | Yes        |
| glTexParameteriv          | Yes        |
| glTexSubImage2D           | Yes        |

### Texture (extensions)

| Name          | Available? |
| ------------- | ---------- |
| glTexVRAMPICA | Yes        |

## GLES 2

### Attributes

| Name                       | Available? |
| -------------------------- | ---------- |
| glBindAttribLocation       | Stubbed    |
| glDisableVertexAttribArray | Yes        |
| glEnableVertexAttribArray  | Yes        |
| glGetActiveAttrib          | Yes        |
| glGetAttribLocation        | Yes        |
| glGetVertexAttribfv        | Yes        |
| glGetVertexAttribiv        | Yes        |
| glGetVertexAttribPointerv  | Yes        |
| glVertexAttrib1f           | Yes        |
| glVertexAttrib2f           | Yes        |
| glVertexAttrib3f           | Yes        |
| glVertexAttrib4f           | Yes        |
| glVertexAttrib1fv          | Yes        |
| glVertexAttrib2fv          | Yes        |
| glVertexAttrib3fv          | Yes        |
| glVertexAttrib4fv          | Yes        |
| glVertexAttribPointer      | Yes        |

### Framebuffer

| Name                                  | Available? |
| ------------------------------------- | ---------- |
| glBindFramebuffer                     | Yes        |
| glBindRenderbuffer                    | Yes        |
| glCheckFramebufferStatus              | Yes        |
| glDeleteFramebuffers                  | Yes        |
| glDeleteRenderbuffers                 | Yes        |
| glFramebufferRenderbuffer             | Yes        |
| glFramebufferTexture2D                | Yes        |
| glGenFramebuffers                     | Yes        |
| glGenRenderbuffers                    | Yes        |
| glGenerateMipmap                      | No         |
| glGetFramebufferAttachmentParameteriv | No         |
| glGetRenderbufferParameteriv          | Yes        |
| glIsFramebuffer                       | Yes        |
| glIsRenderbuffer                      | Yes        |
| glRenderbufferStorage                 | Yes        |

### Shaders

| Name                       | Available? |
| -------------------------- | ---------- |
| glAttachShader             | Yes        |
| glCompileShader            | Stubbed    |
| glCreateProgram            | Yes        |
| glCreateShader             | Yes (*)    |
| glDeleteProgram            | Yes        |
| glDeleteShader             | Yes        |
| glDetachShader             | Yes        |
| glGetAttachedShaders       | Yes        |
| glGetProgramInfoLog        | Stubbed    |
| glGetProgramiv             | Yes        |
| glGetShaderInfoLog         | Stubbed    |
| glGetShaderPrecisionFormat | Stubbed    |
| glGetShaderSource          | Stubbed    |
| glGetShaderiv              | Yes        |
| glIsProgram                | Yes        |
| glIsShader                 | Yes        |
| glLinkProgram              | Yes        |
| glReleaseShaderCompiler    | Stubbed    |
| glShaderBinary             | Yes        |
| glShaderSource             | Stubbed    |
| glUseProgram               | Yes        |
| glValidateProgram          | Yes        |

- (*) Geometry shaders are not currently supported.

### Uniform

| Name                 | Available? |
| -------------------- | ---------- |
| glGetActiveUniform   | Yes        |
| glGetUniformfv       | Yes        |
| glGetUniformiv       | Yes        |
| glGetUniformLocation | Yes        |
| glUniform1f          | Yes        |
| glUniform2f          | Yes        |
| glUniform3f          | Yes        |
| glUniform4f          | Yes        |
| glUniform1fv         | Yes        |
| glUniform2fv         | Yes        |
| glUniform3fv         | Yes        |
| glUniform4fv         | Yes        |
| glUniform1i          | Yes        |
| glUniform2i          | Yes        |
| glUniform3i          | Yes        |
| glUniform4i          | Yes        |
| glUniform1iv         | Yes        |
| glUniform2iv         | Yes        |
| glUniform3iv         | Yes        |
| glUniform4iv         | Yes        |
| glUniformMatrix2fv   | Yes        |
| glUniformMatrix3fv   | Yes        |
| glUniformMatrix4fv   | Yes        |

## GLES 1.1

No.