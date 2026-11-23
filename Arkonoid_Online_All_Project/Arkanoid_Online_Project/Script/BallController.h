#pragma once
#include "Component.h"
#include "../AllComponent.h"

class InputManager;

class BallController : public Component {
private:
	Transform* tr;
	InputManager* inputManager;

	float speed = 15.0f;

	bool isCatched = true;
	vec3 velocity = vec3(speed, 0.0f, 0.0f);

public:
	bool canMoveLeft = true;
	bool canMoveRight = true;

	void Start() override;
	void Update(float deltaTime) override;

	void OnTriggerEnter(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};