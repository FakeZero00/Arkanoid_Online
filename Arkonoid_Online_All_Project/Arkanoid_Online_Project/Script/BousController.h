#pragma once
#include "Component.h"
#include "../AllComponent.h"
#include "Object.h"
#include "BallController.h"

class BousController : public Component {
private:
	Transform* tr;
	InputManager* inputManager;

	Object* ball;
	BallController* ballController;

	float speed = 15.0f;
	bool canMoveLeft = true;
	bool canMoveRight = true;

public:
	void Start() override;
	void Update(float deltaTime) override;

	void OnTriggerEnter(Object* other) override;
	void OnTriggerExit(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};