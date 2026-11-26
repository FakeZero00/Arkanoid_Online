#pragma once
#include <vector>
#include <GL/glew.h>
#include <gl/glm/glm.hpp>
#include <string>
#include "Vertex.h"
using namespace std;

struct SubMeshData {
	string materialName;
	vector<GLuint> indices;
};

void LoadObjMeshData(const vector<vector<string>>& objData, vector<Vertex>& outVertices, vector<SubMeshData>& outSubMeshes);
vector<vec3> GetControlPointsFromObj(vector<vector<string>>& objData);