#include "Mesh.h"
#include <cfloat>
#include <iostream>

//파일 읽는데 필요한 헤더
#include <sstream>
#include <fstream>
using namespace std;

void Mesh::SetupMesh(const vector<Vertex>& vertices, const vector<SubMeshData>& subMeshDataList) {
	minPos = vec3{ FLT_MAX };
	maxPos = vec3{ -FLT_MAX };

	for (const auto& vertex : vertices) {
		minPos = glm::min(minPos, vertex.position);
		maxPos = glm::max(maxPos, vertex.position);
	}

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	// 버텍스 속성 설정
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
	glEnableVertexAttribArray(3);

	// 서브메쉬별 EBO 생성
	subMeshes.clear();
	for (const auto& subData : subMeshDataList) {
		if (subData.indices.empty()) continue;

		SubMesh sub{};
		sub.materialName = subData.materialName;
		sub.indexCount = static_cast<GLsizei>(subData.indices.size());

		glGenBuffers(1, &sub.EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, subData.indices.size() * sizeof(GLuint), subData.indices.data(), GL_STATIC_DRAW);

		subMeshes.push_back(sub);
	}

	glBindVertexArray(0);
}

Mesh::Mesh(const string& objFile) {
	string path = "Assets/" + objFile;

	ifstream in{ path };
	if (not in) {
		cout << "파일을 열 수 없습니다: " << path << endl;
		system("pause");
		exit(1);
	}

	vector<vector<string>> data;
	string line;
	while (getline(in, line)) {
		stringstream ss{ line };
		string word;
		vector<string> words;

		while (ss >> word) words.push_back(word);
		if (!words.empty()) data.push_back(words);
	}

	vector<Vertex> vertices;
	vector<SubMeshData> subMeshDataList;
	LoadObjMeshData(data, vertices, subMeshDataList);

	SetupMesh(vertices, subMeshDataList);
}

void Mesh::Bind() const {
	glBindVertexArray(VAO);
}