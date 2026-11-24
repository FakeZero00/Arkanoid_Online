#pragma once
#include "Component.h"
#include "../AllComponent.h"
#include <string>

class InputManager;

class BlockBehavior : public Component {
private:
	string color;
	int hitCount = 0;
	int blockIndex = 0;		// 스테이지 안의 블록 번호 (줄 * 11 + 칸). 상대 화면에서 같은 블록을 찾는 데 사용
	bool broken = false;	// 같은 프레임에 두 번 깨진 것으로 보내지 않도록

public:
	BlockBehavior() = default;
	BlockBehavior(string& color, int blockIndex = 0) : color(color), blockIndex(blockIndex) {}

	void OnTriggerEnter(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};
