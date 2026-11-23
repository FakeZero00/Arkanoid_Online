#pragma once
#include "Collider.h"
#include "Transform.h"
#include <gl/glm/glm.hpp>
using namespace glm;

class BoxCollider : public Collider {
private:
	Transform* tr;

public:
	vec3 center;
	vec3 size;

	//로컬 공간에서의 min/max (Oriented Bounding Box(OBB)를 위한 로컬 좌표계의 AABB)
	vec3 localMin;
	vec3 localMax;

	//월드 공간에서의 min/max (Axis-Aligned Bounding Box(AABB)를 위한 월드 좌표계의 AABB)
	vec3 minPos;
	vec3 maxPos;

	bool isDebug = false;

	BoxCollider(vec3 center = vec3{ 0.0 }, vec3 size = vec3{ 1.0 });

	bool MouseCollide(vec2 mousePos);

	void RecalculateCollision() override;
	void Start() override;
	void Render(Camera* activeCamera) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);

private:
	void InitDebugRender() override;
};