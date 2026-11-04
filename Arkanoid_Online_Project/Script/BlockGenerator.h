#pragma once
#include "Component.h"
#include "../AllComponent.h"

class BlockGenerator : public Component {
private:
	int stage = 0;

public:
	void Start() override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};