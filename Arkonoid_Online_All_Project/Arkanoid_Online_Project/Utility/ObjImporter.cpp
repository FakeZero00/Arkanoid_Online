#include "ObjImporter.h"
#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <cstdlib> // atoi, atof 사용을 위해 추가
#include <map>

using namespace glm;

void LoadObjMeshData(const vector<vector<string>>& objData, vector<Vertex>& outVertices, vector<SubMeshData>& outSubMeshes) {
	vector<vec3> tempPositions;
	vector<vec2> tempTexCoords;
	vector<vec3> tempNormals;

	outVertices.clear();
	outSubMeshes.clear();

	// 1. v, vt, vn 원본 데이터 수집
	for (const auto& line : objData) {
		if (line.empty()) continue;

		if (line[0] == "v" && line.size() >= 4) {
			tempPositions.push_back(vec3(atof(line[1].c_str()), atof(line[2].c_str()), atof(line[3].c_str())));
		}
		else if (line[0] == "vt" && line.size() >= 3) {
			tempTexCoords.push_back(vec2(atof(line[1].c_str()), atof(line[2].c_str())));
		}
		else if (line[0] == "vn" && line.size() >= 4) {
			tempNormals.push_back(vec3(atof(line[1].c_str()), atof(line[2].c_str()), atof(line[3].c_str())));
		}
	}

	map<string, GLuint> vertexCache;
	map<string, int> subMeshIndexMap; // 마테리얼 이름 -> outSubMeshes 인덱스 매핑
	int currentSubMeshIdx = -1;

	// 2. usemtl 및 면(f) 파싱
	for (const auto& line : objData) {
		if (line.empty()) continue;

		// 새로운 마테리얼 구간 진입 시 해당 서브메쉬를 활성화 (첫 등장 순서대로 인덱스 부여)
		if (line[0] == "usemtl" && line.size() >= 2) {
			const string& matName = line[1];
			auto it = subMeshIndexMap.find(matName);
			if (it == subMeshIndexMap.end()) {
				int newIdx = static_cast<int>(outSubMeshes.size());
				outSubMeshes.push_back(SubMeshData{ matName, {} });
				subMeshIndexMap[matName] = newIdx;
				currentSubMeshIdx = newIdx;
			}
			else {
				currentSubMeshIdx = it->second;
			}
			continue;
		}

		if (line[0] != "f") continue;

		// usemtl 없이 f가 먼저 나오는 OBJ 파일 대비 기본 서브메쉬 생성
		if (currentSubMeshIdx == -1) {
			outSubMeshes.push_back(SubMeshData{ "default", {} });
			subMeshIndexMap["default"] = 0;
			currentSubMeshIdx = 0;
		}

		size_t vertexCount = line.size() - 1;
		if (vertexCount < 3) continue;

		vector<GLuint> faceIndices;
		faceIndices.reserve(vertexCount);

		for (size_t i = 1; i <= vertexCount; ++i) {
			const string& token = line[i];

			auto it = vertexCache.find(token);
			if (it != vertexCache.end()) {
				faceIndices.push_back(it->second);
				continue;
			}

			int vIdx = -1, vtIdx = -1, vnIdx = -1;
			size_t firstSlash = token.find('/');
			size_t secondSlash = (firstSlash != string::npos) ? token.find('/', firstSlash + 1) : string::npos;

			if (firstSlash == string::npos) {
				vIdx = atoi(token.c_str()) - 1;
			}
			else {
				vIdx = atoi(token.substr(0, firstSlash).c_str()) - 1;
				if (secondSlash == string::npos) {
					string vtStr = token.substr(firstSlash + 1);
					if (!vtStr.empty()) vtIdx = atoi(vtStr.c_str()) - 1;
				}
				else {
					string vtStr = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
					if (!vtStr.empty()) vtIdx = atoi(vtStr.c_str()) - 1;

					string vnStr = token.substr(secondSlash + 1);
					if (!vnStr.empty()) vnIdx = atoi(vnStr.c_str()) - 1;
				}
			}

			Vertex vert{};
			if (vIdx >= 0 && vIdx < static_cast<int>(tempPositions.size())) {
				vert.position = tempPositions[vIdx];
				vert.color = vec3(1.0f);
			}
			if (vtIdx >= 0 && vtIdx < static_cast<int>(tempTexCoords.size())) {
				vert.texCoord = tempTexCoords[vtIdx];
			}
			else {
				vert.texCoord = vec2(0.0f);
			}
			if (vnIdx >= 0 && vnIdx < static_cast<int>(tempNormals.size())) {
				vert.normal = glm::normalize(tempNormals[vnIdx]);
			}
			else {
				vert.normal = vec3(0.0f, 1.0f, 0.0f);
			}

			GLuint newIndex = static_cast<GLuint>(outVertices.size());
			outVertices.push_back(vert);
			vertexCache[token] = newIndex;
			faceIndices.push_back(newIndex);
		}

		// 현재 활성화된 서브메쉬의 인덱스 버퍼에 삼각형 추가
		auto& targetIndices = outSubMeshes[currentSubMeshIdx].indices;
		for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
			targetIndices.push_back(faceIndices[0]);
			targetIndices.push_back(faceIndices[i]);
			targetIndices.push_back(faceIndices[i + 1]);
		}
	}
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