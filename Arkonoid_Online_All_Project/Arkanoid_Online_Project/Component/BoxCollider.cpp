#include "BoxCollider.h"
#include "Object.h"
#include "MeshRenderer3D.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
#include <cfloat>
#include <gl/glm/gtc/type_ptr.hpp>
#include <gl/glew.h>

//main에서 선언된 shaders 맵을 extern으로 참조
extern map<string, GLuint> shaders;

BoxCollider::BoxCollider(vec3 center, vec3 size) : Collider(ColliderType::Box), center(center), size(size) {
	RecalculateCollision();
	InitDebugRender();
}

bool BoxCollider::MouseCollide(vec2 mousePos) {
	//마우스 좌표를 이용해 충돌 여부 판단
	if (mousePos.x >= minPos.x && mousePos.x <= maxPos.x &&
		mousePos.y >= minPos.y && mousePos.y <= maxPos.y) {
		return true;
	}
	return false;
}

void BoxCollider::RecalculateCollision() {
	if (not gameObject) return;

	if (!tr) tr = gameObject->GetComponent<Transform>();

	//메시 데이터가 없을 때의 기본 박스 범위 설정
	vec3 meshMin{ -0.5f };
	vec3 meshMax{ 0.5f };

	MeshRenderer3D* mr = gameObject->GetComponent<MeshRenderer3D>();
	if (mr && mr->mesh) {
		meshMin = mr->mesh->minPos;
		meshMax = mr->mesh->maxPos;
	}

	//Mesh와 Collider의 Center, Size를 고려한 로컬 좌표계에서의 minPos, maxPos 계산
	vec3 meshCenter = (meshMin + meshMax) / 2.0f;
	vec3 meshExtent = (meshMax - meshMin) / 2.0f;

	localMin = (meshCenter + center) - (meshExtent * size);
	localMax = (meshCenter + center) + (meshExtent * size);

	//로컬 바운딩 박스의 8개 꼭짓점 좌표 배열 생성
	vec3 corners[8] = {
		vec3(localMin.x, localMin.y, localMin.z),
		vec3(localMax.x, localMin.y, localMin.z),
		vec3(localMax.x, localMax.y, localMin.z),
		vec3(localMin.x, localMax.y, localMin.z),
		vec3(localMin.x, localMin.y, localMax.z),
		vec3(localMax.x, localMin.y, localMax.z),
		vec3(localMax.x, localMax.y, localMax.z),
		vec3(localMin.x, localMax.y, localMax.z)
	};

	//월드 공간의 min/max 좌표 계산
	minPos = vec3(FLT_MAX);
	maxPos = vec3(-FLT_MAX);

	for (int i = 0; i < 8; ++i) {
		vec4 worldPos = tr->worldMatrix * vec4(corners[i], 1.0f);

		minPos = glm::min(minPos, vec3(worldPos));
		maxPos = glm::max(maxPos, vec3(worldPos));
	}
}

void BoxCollider::Start() {
	tr = gameObject->GetComponent<Transform>();
	
	RecalculateCollision();

	gameObject->ctx.pendingCollisionObjects.push_back(this);
}

void BoxCollider::Render(Camera* activeCamera) {
	if (not isDebug) return;

	RecalculateCollision();

	GLuint debugShader = shaders["Debug"];
	glUseProgram(debugShader);

	//view, projection 행렬 설정
	if (activeCamera != nullptr) {
		GLuint viewLoc = glGetUniformLocation(debugShader, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(activeCamera->GetViewMatrix()));
		GLuint projLoc = glGetUniformLocation(debugShader, "proj");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(activeCamera->GetProjectionMatrix()));
	}

	//오브젝트의 worldMatrix를 그대로 모델 행렬에 넘겨 회전
	GLuint modelLoc = glGetUniformLocation(debugShader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(tr->worldMatrix));

	//월드 좌표가 아닌 로컬 좌표로 8개 꼭짓점 버퍼 구성
	float x1 = localMin.x, y1 = localMin.y, z1 = localMin.z;
	float x2 = localMax.x, y2 = localMax.y, z2 = localMax.z;
	float g = 1.0f;

	float vertices[] = {
		x1, y1, z1,		0.0f, g, 0.0f,
		x2, y1, z1,		0.0f, g, 0.0f,
		x2, y2, z1,		0.0f, g, 0.0f,
		x1, y2, z1,		0.0f, g, 0.0f,
		x1, y1, z2,		0.0f, g, 0.0f,
		x2, y1, z2,		0.0f, g, 0.0f,
		x2, y2, z2,		0.0f, g, 0.0f,
		x1, y2, z2,		0.0f, g, 0.0f
	};

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void BoxCollider::InitDebugRender() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	//VBO 초기화
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 8 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//EBO 초기화
	GLuint indices[] = {
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
	};
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

Component* BoxCollider::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	if (!props.contains("center") || !props.contains("size")) {
		cout << obj->name << " 오브젝트의 BoxCollider 컴포넌트에 center와 size 속성이 필요합니다." << endl;
	}
	auto center = props["center"];
	auto size = props["size"];
	BoxCollider* boxCollider = obj->AddComponent<BoxCollider>(vec3{ center[0], center[1], center[2] },
		vec3(size[0], size[1], size[2]));
	if (props.contains("isTrigger")) boxCollider->isTrigger = props["isTrigger"];
	if (props.contains("isDebug")) boxCollider->isDebug = props["isDebug"];

	return boxCollider;
}

REGISTER_COMPONENT(BoxCollider);