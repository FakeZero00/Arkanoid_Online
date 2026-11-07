#pragma once
#include "Component.h"
#include "../AllComponent.h"

class InputManager;

class BlockBehavior : public Component {
public:
	void OnTriggerEnter(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};