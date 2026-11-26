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
	bool isLocal = true;	// true = 내 판 (충돌로 깨지고 송신), false = 상대 판 (충돌 무시, 받은 이벤트로만 바뀜)

	string BallName = "pBall";

public:
	BlockBehavior() = default;
	BlockBehavior(string& color, string& ballName, int blockIndex = 0, bool isLocal = true)
		: color(color), blockIndex(blockIndex), isLocal(isLocal), BallName(ballName) {}

	void OnTriggerEnter(Object* other) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};
