#include "BlockBehavior.h"
#include "Object.h"
#include "GameNet.h"

void BlockBehavior::OnTriggerEnter(Object* other)
{
	if (!isLocal) return;	// 상대 판 블록은 서버에서 받은 BLOCK_BROKEN으로만 지움 (11/22 수신)
	if (broken) return;		// 이미 깨짐 (지워지기 전에 공이 또 닿은 경우)

	if (other->name == BallName) {
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

	if (props.contains("BallName")) blockBehavior->BallName = props["BallName"];

	return blockBehavior;
}

REGISTER_COMPONENT(BlockBehavior);