#include "Mesh.h"
#include "ObjImporter.h"
#include <cfloat>
#include <iostream>

//파일 읽는데 필요한 헤더
#include <sstream>
#include <fstream>
using namespace std;

Mesh::Mesh(const vector<Vertex>& vertices, const vector<GLuint>& indices) {
	indexCount = indices.size();

	minPos = vec3{ FLT_MAX };
	maxPos = vec3{ -FLT_MAX };

	//버텍스 데이터에서 최소, 최대 좌표 계산
	for (const auto& vertex : vertices) {
		minPos = glm::min(minPos, vertex.position);
		maxPos = glm::max(maxPos, vertex.position);
	}

	//VAO 객체 생성 및 바인딩
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//VBO 객체 생성 및 바인딩
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//EBO 객체 생성 및 바인딩 및 데이터 설정
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	//버텍스 데이터 설정
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	//버텍스 좌표: 속성 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//버텍스 색상: 속성 1
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);

	// VAO 바인딩 해제
	glBindVertexArray(0);
}

Mesh::Mesh(const string& objFile) {
	string path = "Assets/" + objFile;

	ifstream in{ path };
	if (not in) {
		cout << "파일을 열 수 없습니다." << endl;
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

	vector<Vertex> vertices = GetVerticesFromObj(data);
	vector<GLuint> indices = GetIndicesFromObj(data);

	indexCount = indices.size();

	minPos = vec3{ FLT_MAX };
	maxPos = vec3{ -FLT_MAX };

	//버텍스 데이터에서 최소, 최대 좌표 계산
	for (const auto& vertex : vertices) {
		minPos = glm::min(minPos, vertex.position);
		maxPos = glm::max(maxPos, vertex.position);
	}

	//VAO 객체 생성 및 바인딩
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//VBO 객체 생성 및 바인딩
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//EBO 객체 생성 및 바인딩 및 데이터 설정
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	//버텍스 데이터 설정
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	//버텍스 좌표: 속성 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//버텍스 색상: 속성 1
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);

	// VAO 바인딩 해제
	glBindVertexArray(0);
}

void Mesh::Bind() const {
	glBindVertexArray(VAO);
}