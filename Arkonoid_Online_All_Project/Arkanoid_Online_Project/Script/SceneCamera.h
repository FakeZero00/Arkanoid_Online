#pragma once
#include "Component.h"

class SceneCamera : public Component {
public:
	float walkSpeed = 5.0f;
	float dashSpeed = 15.0f;
	float sensitivity = 100.0f;

	float lastMouseX = 0.0f;
	float lastMouseY = 0.0f;

	float yaw;
	float pitch;

	void Awake() override;
	void Update(float deltaTime) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};