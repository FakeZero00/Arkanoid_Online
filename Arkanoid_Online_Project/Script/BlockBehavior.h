#pragma once
#include "Component.h"
#include "../AllComponent.h"
#include <string>

class InputManager;

class BlockBehavior : public Component {
private:
	string color;
	int hitCount = 0;

public:
	BlockBehavior() = default;
	BlockBehavior(string& color) : color(color) {}

	void OnTriggerEnter(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};