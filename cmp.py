c3d_list = []
with open("C3D.txt", "r") as f:
    c3d_list = f.readlines()

gl_list = []
with open("GL.txt", "r") as f:
    gl_list = f.readlines()

in_c3d = []
in_gl = []

for line in c3d_list:
    found = False
    for gline in gl_list:
        if line == gline:
            found = True
            break

    if not found:
        in_c3d.append(line)

for line in gl_list:
    found = False
    for cline in c3d_list:
        if line == cline:
            found = True
            break

    if not found:
        in_gl.append(line)

with open("C3D_only.txt", "w") as out:
    out.writelines(in_c3d)

with open("GL_only.txt", "w") as out:
    out.writelines(in_gl)