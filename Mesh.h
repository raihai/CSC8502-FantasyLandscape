#pragma once
#include "OGLRenderer.h"

enum MeshBuffer {
	VERTIX_BUFFER, COLOUR_BUFFER, NAX_BUFFER
};

class Mesh {
public:
	Mesh(void);
	~Mesh(void);

	virtual void Draw();
	static Mesh* GenrateTriangle();

protected:
	void BufferData();

	GLuint arrayObject;
	GLuint bufferObject[MAX_BUFFER];
	GLuint numVertices;
	GLuint type;

	Vector3* vertices;
	Vector4* colours;

};