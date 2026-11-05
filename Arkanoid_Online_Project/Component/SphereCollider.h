#pragma once
#include "Collider.h"
#include "Transform.h"
#include <gl/glm/glm.hpp>
using namespace glm;

class SphereCollider : public Collider {
private:
	Transform* tr;

public:
	vec3 center;
	float radius;

	vec3 worldCenter;
	float worldRadius;

	bool isDebug = false;

	SphereCollider(vec3 center = vec3{ 0.0 }, float radius = 1.0f);

	void RecalculateCollision() override;
	void Start() override;
	void Render(Camera* activeCamera) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);

private:
	void InitDebugRender() override;
};