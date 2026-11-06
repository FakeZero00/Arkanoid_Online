#pragma once
#include "Component.h"
#include "../AllComponent.h"

class InputManager;

class BallController : public Component {
private:
	Transform* tr;
	InputManager* inputManager;

	float speed = 15.0f;

public:
	void Start() override;
	void Update(float deltaTime) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};