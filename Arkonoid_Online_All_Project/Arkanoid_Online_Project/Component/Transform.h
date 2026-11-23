#pragma once
#include <vector>
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/quaternion.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include "Component.h"
using namespace std;
using namespace glm;

class Transform : public Component {
public:
	vec3 position;
	quat rotation;
	vec3 scale;

	//최종적으로 셰이더에 넘겨줄 월드 변환 행렬
	mat4 worldMatrix;

	//편의성 변수들
	vec3 worldPosition;
	vec3 right;
	vec3 up;
	vec3 forward;
	vec3 EulerAngles = vec3{ 0.0f };

	Transform* parent = nullptr;
	vector<Transform*> children = vector<Transform*>{}; //빈 자식 벡터 생성

	Transform();
	
	void SetLocalPosition(float x, float y, float z);
	void SetLocalPosition(vec3 newPos);
	void Translate(float x, float y, float z, bool isWorld = false);
	void Translate(vec3 delta, bool isWorld = false);

	void SetLocalRotation(float x, float y, float z);
	void SetLocalRotation(quat newRot);
	void Rotate(float x, float y, float z, bool isWorld = false);

	void SetLocalScale(float x, float y, float z);

	void CalculateWorldMatrix();
	void SetParent(Transform* newParent);
};