#include "ObjImporter.h"

#include "Mesh.h"
#include <fstream>
#include <sstream>

vector<Vertex> GetVerticesFromObj(vector<vector<string>>& objData) {
	vector<Vertex> vertices;
	
	for (auto& line : objData) {
		if (line.empty()) continue;
		if (line[0] == "v") {
			double x = stod(line[1]);
			double y = stod(line[2]);
			double z = stod(line[3]);

			double r = x + 1.0 * 0.5f;
			double g = y + 1.0 * 0.5f;
			double b = z + 1.0 * 0.5f;

			vertices.push_back({ vec3(x, y, z), vec3(r, g, b) });
		}
	}
	return vertices;
}

vector<GLuint> GetIndicesFromObj(vector<vector<string>>& objData) {
	vector<GLuint> indices;

	for (auto& line : objData) {
		if (line.empty()) continue;
		if (line[0] == "f") {
			size_t vertexCount = line.size() - 1; // 'f'를 제외한 페이스 정점 개수
			if (vertexCount < 3) continue;

			// 1. 해당 페이스의 모든 정점 인덱스 파싱
			vector<GLuint> faceIndices;
			faceIndices.reserve(vertexCount);

			for (size_t i = 1; i <= vertexCount; ++i) {
				size_t slashPos = line[i].find('/');
				int idx = (slashPos != string::npos)
					? stoi(line[i].substr(0, slashPos)) - 1
					: stoi(line[i]) - 1;
				faceIndices.push_back(static_cast<GLuint>(idx));
			}

			// 2. 부채꼴 삼각 분할 (Triangle Fan)
			// N개의 정점을 가진 다각형을 (N-2)개의 삼각형으로 분할
			// (v0, v1, v2), (v0, v2, v3), (v0, v3, v4)...
			for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
				indices.push_back(faceIndices[0]);
				indices.push_back(faceIndices[i]);
				indices.push_back(faceIndices[i + 1]);
			}
		}
	}
	return indices;
}

vector<vec3> GetControlPointsFromObj(vector<vector<string>>& objData) {
	vector<vec3> controlPoints;
	for (auto& line : objData) {
		if (line.empty()) continue;
		if (line[0] == "v") {
			double x = stod(line[1]);
			double y = stod(line[2]);
			double z = stod(line[3]);
			controlPoints.push_back(vec3(x, y, z));
		}
	}
	return controlPoints;
}