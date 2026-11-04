#pragma once
#include "Component.h"
#include "../AllComponent.h"

class TestScript : public Component {
private:
	Transform* tr;
	RigidBody* rb;

public:
	void Start() override;
	void Update(float deltaTime) override;

	void OnTriggerStay(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};