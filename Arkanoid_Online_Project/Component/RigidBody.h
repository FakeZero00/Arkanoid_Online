#pragma once
#include "Component.h"
#include "Transform.h"
#include "ForceMode.h"
#include <gl/glm/glm.hpp>
using namespace glm;

class RigidBody : public Component {
private:
	Transform* tr;
	vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
	vec3 forceAccumulator = vec3{ 0.0f };

public:
	float mass = 1.0;
	bool useGravity = true;
	bool isKinematic = false;

	vec3 velocity = vec3{ 0.0f };

	void AddForce(vec3 force, ForceMode mode = Force);

	void Start() override;
	void Update(float deltaTime) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};