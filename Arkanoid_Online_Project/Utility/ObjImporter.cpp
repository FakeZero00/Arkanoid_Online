#include "ObjImporter.h"
#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <cstdlib> // atoi, atof 사용을 위해 추가

using namespace glm;

vector<Vertex> GetVerticesFromObj(vector<vector<string>>& objData) {
	vector<vec3> tempPositions;
	vector<vec3> tempNormals;

	// 1. 정점(v)과 법선(vn) 데이터 파싱
	for (auto& line : objData) {
		if (line.empty()) continue;

		if (line[0] == "v" && line.size() >= 4) {
			tempPositions.push_back(vec3(atof(line[1].c_str()), atof(line[2].c_str()), atof(line[3].c_str())));
		}
		else if (line[0] == "vn" && line.size() >= 4) {
			tempNormals.push_back(vec3(atof(line[1].c_str()), atof(line[2].c_str()), atof(line[3].c_str())));
		}
	}

	// 2. Vertex 배열 초기화 (위치 및 기본 색상 설정)
	vector<Vertex> vertices(tempPositions.size());
	for (size_t i = 0; i < tempPositions.size(); ++i) {
		vertices[i].position = tempPositions[i];

		float r = tempPositions[i].x + 0.5f;
		float g = tempPositions[i].y + 0.5f;
		float b = tempPositions[i].z + 0.5f;
		vertices[i].color = vec3(r, g, b);

		vertices[i].normal = vec3(0.0f, 0.0f, 0.0f);
	}

	// 3. 각 정점별로 연결된 모든 면의 법선(vn)을 수집하기 위한 임시 컨테이너
	vector<vector<vec3>> vertexNormals(tempPositions.size());

	for (auto& line : objData) {
		if (line.empty()) continue;

		if (line[0] == "f") {
			for (size_t i = 1; i < line.size(); ++i) {
				string vertexDef = line[i];

				size_t firstSlash = vertexDef.find('/');
				size_t secondSlash = (firstSlash != string::npos) ? vertexDef.find('/', firstSlash + 1) : string::npos;

				if (firstSlash != string::npos && firstSlash > 0) {
					int vIdx = atoi(vertexDef.substr(0, firstSlash).c_str()) - 1;
					int vnIdx = -1;

					// vn 인덱스 추출
					if (secondSlash != string::npos && secondSlash + 1 < vertexDef.length()) {
						string vnStr = vertexDef.substr(secondSlash + 1);
						if (!vnStr.empty()) {
							vnIdx = atoi(vnStr.c_str()) - 1;
						}
					}

					// 유효한 범위일 경우 해당 정점에 노말 누적
					if (vIdx >= 0 && vIdx < static_cast<int>(tempPositions.size()) &&
						vnIdx >= 0 && vnIdx < static_cast<int>(tempNormals.size())) {
						vertexNormals[vIdx].push_back(tempNormals[vnIdx]);
					}
				}
			}
		}
	}

	// 4. 수집된 노말들의 평균을 계산하여 최종 정점 노말에 할당
	for (size_t i = 0; i < tempPositions.size(); ++i) {
		if (!vertexNormals[i].empty()) {
			vec3 sumNorm(0.0f);
			for (const auto& n : vertexNormals[i]) {
				sumNorm += n;
			}
			vertices[i].normal = glm::normalize(sumNorm);
		}
		else {
			vertices[i].normal = vec3(0.0f, 1.0f, 0.0f); // Fallback
		}
	}

	return vertices;
}

vector<GLuint> GetIndicesFromObj(vector<vector<string>>& objData) {
	vector<GLuint> indices;

	for (auto& line : objData) {
		if (line.empty()) continue;

		if (line[0] == "f") {
			size_t vertexCount = line.size() - 1;
			if (vertexCount < 3) continue;

			vector<GLuint> faceIndices;
			faceIndices.reserve(vertexCount);

			for (size_t i = 1; i <= vertexCount; ++i) {
				size_t slashPos = line[i].find('/');
				int idx = -1;

				if (slashPos != string::npos && slashPos > 0) {
					idx = atoi(line[i].substr(0, slashPos).c_str()) - 1;
				}
				else if (slashPos == string::npos) {
					idx = atoi(line[i].c_str()) - 1;
				}

				if (idx >= 0) {
					faceIndices.push_back(static_cast<GLuint>(idx));
				}
			}

			// 부채꼴 삼각 분할 (Triangle Fan)
			if (faceIndices.size() >= 3) {
				for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
					indices.push_back(faceIndices[0]);
					indices.push_back(faceIndices[i]);
					indices.push_back(faceIndices[i + 1]);
				}
			}
		}
	}
	return indices;
}

vector<vec3> GetControlPointsFromObj(vector<vector<string>>& objData) {
	vector<vec3> controlPoints;
	for (auto& line : objData) {
		if (line.empty()) continue;

		if (line[0] == "v" && line.size() >= 4) {
			double x = atof(line[1].c_str());
			double y = atof(line[2].c_str());
			double z = atof(line[3].c_str());
			controlPoints.push_back(vec3(x, y, z));
		}
	}
	return controlPoints;
}