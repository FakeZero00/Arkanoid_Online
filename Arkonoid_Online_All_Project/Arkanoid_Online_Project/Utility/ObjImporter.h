#pragma once
#include <vector>
#include <GL/glew.h>
#include <gl/glm/glm.hpp>
#include <string>
#include "Vertex.h"
using namespace std;

vector<Vertex> GetVerticesFromObj(vector<vector<string>>& objData);
vector<GLuint> GetIndicesFromObj(vector<vector<string>>& objData);
vector<vec3> GetControlPointsFromObj(vector<vector<string>>& objData);