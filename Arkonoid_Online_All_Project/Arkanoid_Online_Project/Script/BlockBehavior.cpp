#include "BlockBehavior.h"
#include "Object.h"
#include "GameNet.h"

void BlockBehavior::OnTriggerEnter(Object* other)
{
	if (broken) return;

	if (other->name == "Ball") {
		hitCount++;

		int maxHp = (color == "Silver") ? 2 : 1;
		if (hitCount >= maxHp) {
			broken = true;
			// 깨질 때는 BLOCK_BROKEN 하나만 보냄 (상대 화면은 이것만 보고 블록을 지움)
			GameNet::SendBlockBroken((uint16_t)blockIndex,
				(color == "Silver") ? BLOCK_SCORE_SILVER : BLOCK_SCORE_NORMAL);
			gameObject->Destroy();
		}
		else {
			// 맞았지만 안 깨짐: 남은 내구도를 보냄
			GameNet::SendBlockHit((uint16_t)blockIndex, maxHp - hitCount);
		}
	}
}

Component* BlockBehavior::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	BlockBehavior* blockBehavior = obj->AddComponent<BlockBehavior>();

	return blockBehavior;
}

REGISTER_COMPONENT(BlockBehavior);
