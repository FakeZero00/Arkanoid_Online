#pragma once
#include <vector>
#include <gl/glew.h>
#include <string>
#include "ObjImporter.h"
#include "Vertex.h"
using namespace std;
using namespace glm;

struct SubMesh {
	string materialName;
	GLuint EBO;
	GLsizei indexCount;
};

class Mesh {
private:
	void SetupMesh(const vector<Vertex>& vertices, const vector<SubMeshData>& subMeshDataList);

public:
	GLuint VAO, VBO;
	vector<SubMesh> subMeshes;

	vec3 minPos;
	vec3 maxPos;

	Mesh(const string& objFile);
	void Bind() const;
};