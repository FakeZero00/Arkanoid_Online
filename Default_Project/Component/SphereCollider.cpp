#include "SphereCollider.h"
#include "Object.h"
#include "Transform.h"
#include "Camera.h"
#include <vector>
#include <algorithm>
#include <gl/glew.h>
#include <gl/glm/gtc/type_ptr.hpp>

extern map<string, GLuint> shaders;

SphereCollider::SphereCollider(vec3 center, float radius) : Collider(ColliderType::Sphere), center(center), radius(radius) {
	RecalculateCollision();
	InitDebugRender();
}

void SphereCollider::RecalculateCollision() {
	if (!gameObject) return;

	if (!tr) tr = gameObject->GetComponent<Transform>();

	// 1. 월드 좌표계 중심 계산
	worldCenter = vec3(tr->worldMatrix * vec4(center, 1.0f));

	// 2. 월드 스케일 중 가장 큰 값을 가져와서 반지름에 적용 (찌그러진 타원 방지)
	vec3 scaleX = vec3(tr->worldMatrix[0]);
	vec3 scaleY = vec3(tr->worldMatrix[1]);
	vec3 scaleZ = vec3(tr->worldMatrix[2]);
	float maxScale = glm::max(length(scaleX), glm::max(length(scaleY), length(scaleZ)));

	worldRadius = radius * maxScale;
}

void SphereCollider::InitDebugRender() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	
	vector<float> vertices;
	int segments = 36; // 원을 몇 각형으로 그릴지
	float angleStep = glm::radians(360.0f / segments);
	vec3 green = vec3(0.0f, 1.0f, 0.0f);

	// XY, XZ, YZ 3개의 원을 그려서 구체 프레임을 만듦
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < segments; j++) {
			float a = j * angleStep;
			float b = (j + 1) * angleStep;

			vec3 p1, p2;
			if (i == 0) { // XY 평면
				p1 = vec3(cos(a), sin(a), 0.0f); p2 = vec3(cos(b), sin(b), 0.0f);
			}
			else if (i == 1) { // XZ 평면
				p1 = vec3(cos(a), 0.0f, sin(a)); p2 = vec3(cos(b), 0.0f, sin(b));
			}
			else { // YZ 평면
				p1 = vec3(0.0f, cos(a), sin(a)); p2 = vec3(0.0f, cos(b), sin(b));
			}
			vertices.push_back(p1.x); vertices.push_back(p1.y); vertices.push_back(p1.z);
			vertices.push_back(green.x); vertices.push_back(green.y); vertices.push_back(green.z);
			vertices.push_back(p2.x); vertices.push_back(p2.y); vertices.push_back(p2.z);
			vertices.push_back(green.x); vertices.push_back(green.y); vertices.push_back(green.z);
		}
	}

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void SphereCollider::Render(Camera* activeCamera) {
	if (!isDebug) return;
	RecalculateCollision();

	GLuint debugShader = shaders["Debug"];
	glUseProgram(debugShader);

	if (activeCamera != nullptr) {
		GLuint viewLoc = glGetUniformLocation(debugShader, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(activeCamera->GetViewMatrix()));
		GLuint projLoc = glGetUniformLocation(debugShader, "proj");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(activeCamera->GetProjectionMatrix()));
	}

	// 구체의 중심과 반지름을 모델 행렬에 적용
	mat4 model = mat4(1.0f);
	model = translate(model, worldCenter);
	model = scale(model, vec3(worldRadius));
	glUniformMatrix4fv(glGetUniformLocation(debugShader, "model"), 1, GL_FALSE, value_ptr(model));

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, 36 * 3 * 2); // 3개의 원 * 36세그먼트 * 2(점)
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}

void SphereCollider::Start() {
	tr = gameObject->GetComponent<Transform>();

	RecalculateCollision();

	gameObject->ctx.pendingCollisionObjects.push_back(this);
}

Component* SphereCollider::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	if (!props.contains("center") || !props.contains("radius")) {
		cout << obj->name << " 오브젝트의 SphereCollider 컴포넌트에 center와 radius 속성이 필요합니다." << endl;
	}
	auto center = props["center"];
	float radius = props["radius"];
	SphereCollider* sphereCollider = obj->AddComponent<SphereCollider>(vec3{ center[0], center[1], center[2] }, radius);

	if (props.contains("isTrigger")) sphereCollider->isTrigger = props["isTrigger"];
	if (props.contains("isDebug")) sphereCollider->isDebug = props["isDebug"];
	return sphereCollider;
}

REGISTER_COMPONENT(SphereCollider);