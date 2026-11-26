#pragma once
#include "Component.h"
#include "../AllComponent.h"

class BlockGenerator : public Component {
private:
	int stage = 0;

	string BallName = "pBall";
	bool isLocal = true;	// true = 내 판, false = 상대 판 (Default_Scene.json에서 지정)

public:
	void Start() override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};