#ifndef _LENNY_H
#define _LENNY_H

typedef struct {
	float x, y, z;
	float nx, ny, nz;
} vertex;

extern const vertex g_VertexList[3345];
#define NUM_VERTICES (sizeof(g_VertexList)/sizeof(vertex))

#endif /* _LENNY_H */