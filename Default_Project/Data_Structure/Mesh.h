#pragma once
#include <vector>
#include <gl/glew.h>
#include <string>
#include "Vertex.h"
using namespace std;
using namespace glm;

class Mesh {
public:
	GLuint VAO, VBO, EBO;
	GLsizei indexCount;

	vec3 minPos;
	vec3 maxPos;

	Mesh(const vector<Vertex>& vertices, const vector<GLuint>& indices);
	Mesh(const string& objFile);
	void Bind() const;
};