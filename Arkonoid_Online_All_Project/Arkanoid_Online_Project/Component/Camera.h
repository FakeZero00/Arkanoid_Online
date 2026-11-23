#pragma once
#include "Component.h"
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
using namespace glm;

class Camera : public Component {
public:
	float fov = 60.0f;
	float aspectRatio = 1600.0f / 900.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	float width;
	float height;

	int priority = 0; // 카메라 우선순위, 값이 높을수록 우선순위가 높음
	bool isOrthographic = false;

	//뷰 포트 별 Clear 설정
	bool clearColorBuffer = true;
	bool clearDepthBuffer = true;
	vec4 backgroundColor = vec4(0.2f, 0.2f, 0.2f, 1.0f);

	//정규화 뷰포트 영역[x, y, width, height]
	//(x, y = 0, 0은 좌측 하단)
	vec4 viewportRect = vec4(0.0f, 0.0f, 1.0f, 1.0f);

	Camera(float width, float height);

	void Start() override;

	mat4 GetViewMatrix();
	mat4 GetProjectionMatrix();

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};