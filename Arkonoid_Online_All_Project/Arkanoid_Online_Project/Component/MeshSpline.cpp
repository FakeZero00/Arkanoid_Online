#include "MeshSpline.h"
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <gl/glew.h>
#include <gl/glm/gtc/type_ptr.hpp>
#include "ObjImporter.h"
#include "Object.h"
#include "Transform.h"
#include "Camera.h"
using namespace std;

//main에서 선언된 shaders 맵을 extern으로 참조
extern map<string, GLuint> shaders;

MeshSpline::MeshSpline(string& pointObj, vec4 color, float lineWidth) : color(color), lineWidth(lineWidth) {
	vector<vec3> points;
	//pointMesh 파일에서 controlPoints 읽기
	string path = "Assets/" + pointObj;

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

	vector<vec3> vertices = GetControlPointsFromObj(data);
	vertices.push_back(vertices[0]); //마지막 점을 첫 번째 점과 연결하여 닫힌 루프를 만듦
	controlPoints = vertices;

	InitRender();
}

void MeshSpline::Render(Camera* activeCamera) {
	GLuint splineShader = shaders["Spline"];
	glUseProgram(splineShader);

	//controlPoints의 월드 좌표 계산을 위해 model 행렬을 셰이더에 전달
	GLuint modelLoc = glGetUniformLocation(splineShader, "model");
	Transform* tr = gameObject->GetComponent<Transform>();

	// View, Projection 행렬 전달
	if (activeCamera != nullptr) {
		GLuint viewLoc = glGetUniformLocation(splineShader, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(activeCamera->GetViewMatrix()));

		GLuint projLoc = glGetUniformLocation(splineShader, "proj");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(activeCamera->GetProjectionMatrix()));
	}

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(tr->worldMatrix));

	//Color를 셰이더에 전달
	GLuint colorLoc = glGetUniformLocation(splineShader, "color");
	glUniform4fv(colorLoc, 1, value_ptr(color));

	//controlPoints를 VBO에 업데이트
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * 3 * sizeof(float), controlPoints.data(), GL_DYNAMIC_DRAW);

	//Spline 그리기
	glLineWidth(lineWidth);
	glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());
	glLineWidth(1.0f); //라인 두께를 기본값으로 되돌림
	glBindVertexArray(0);
}

void MeshSpline::InitRender() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void MeshSpline::SetColor(vec4 newColor) {
	color = newColor;
}

Component* MeshSpline::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	if (props.contains("pointObj") == false || props.contains("color") == false) {
		cout << obj->name << " 오브젝트의 MeshSpline 컴포넌트에 pointObj와 color 속성이 필요합니다." << endl;
		return nullptr;
	}
	string pointObj = props["pointObj"];
	vec4 colorVec{ props["color"][0], props["color"][1], props["color"][2], props["color"][3] };
	MeshSpline* meshSpline = obj->AddComponent<MeshSpline>(pointObj, colorVec);
	if (props.contains("lineWidth")) meshSpline->lineWidth = props["lineWidth"];
	return meshSpline;
}

REGISTER_COMPONENT(MeshSpline)