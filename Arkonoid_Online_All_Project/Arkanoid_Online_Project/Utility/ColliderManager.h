#pragma once
#include "AppContext.h"
#include "Collider.h"
#include "BoxCollider.h"
#include "SphereCollider.h"

class ColliderManager {
public:
	ColliderManager(AppContext& ctx);

	bool CheckCollision(Collider* colA, Collider* colB);

	bool CheckBoxBox(BoxCollider* colA, BoxCollider* colB);
	bool CheckSphereSphere(SphereCollider* colA, SphereCollider* colB);
	bool CheckBoxSphere(BoxCollider* box, SphereCollider* sphere);

	void Update();

private:
	AppContext& ctx;

	//마찰 계수 결합 함수
	float CombineFriction(float coefA, float coefB);
	//반발 계수 결합 함수
	float CombineRestitution(float restA, float restB);

	//마찰 해결
	void ResolveCollisionWithFriction(Collider* colA, Collider* colB, vec3 collisionNormal);

	//물리 충돌 해소
	void ResolveCollision(Collider* colA, Collider* colB, vec3 collisionNormal, float penetration);
};