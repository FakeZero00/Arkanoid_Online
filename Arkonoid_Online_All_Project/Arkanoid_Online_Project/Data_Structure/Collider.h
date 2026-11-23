#pragma once
#include "Component.h"
#include <vector>
#include <gl/glew.h>
using namespace std;

enum class ColliderType {
	Box,
	Sphere
};

class Collider : public Component {
public:
	ColliderType type;

	//마찰 계수 및 반발 계수
	float dynamicFriction = 0.6f;
	float staticFriction = 0.6f;
	float bounciness = 0.0f;

	bool isTrigger = false; //트리거 여부 (충돌은 감지하지만 물리적 반응은 없음)

	vector<Collider*> collidingColliders;

	//디버그 렌더링용 버퍼
	GLuint VAO = 0, VBO = 0, EBO = 0;

	Collider(ColliderType type) : type(type) {}
	virtual ~Collider() {
		if (VAO) glDeleteVertexArrays(1, &VAO);
		if (VBO) glDeleteBuffers(1, &VBO);
		if (EBO) glDeleteBuffers(1, &EBO);
	}

	//트랜스폼 업데이트 시 콜라이더의 위치를 갱신하는 함수
	virtual void RecalculateCollision() {}

private:
	//디버그 렌더링용 초기화 함수
	virtual void InitDebugRender() {}
};