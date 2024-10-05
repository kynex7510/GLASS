#ifndef _LENNY_H
#define _LENNY_H

typedef struct {
	float x, y, z;
	float nx, ny, nz;
} Vertex;

extern const Vertex g_VertexList[3345];
#define NUM_VERTICES (sizeof(g_VertexList)/sizeof(Vertex))

#endif /* _LENNY_H */