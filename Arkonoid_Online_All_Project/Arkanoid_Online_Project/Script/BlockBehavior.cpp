#include "BlockBehavior.h"
#include "Object.h"

void BlockBehavior::OnTriggerEnter(Object* other)
{
	if (other->name == BallName) {
		hitCount++;

		if (color == "Silver") {
			if(hitCount >= 2) gameObject->Destroy();
		}
		else gameObject->Destroy();
	}
}

Component* BlockBehavior::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	BlockBehavior* blockBehavior = obj->AddComponent<BlockBehavior>();

	if (props.contains("BallName")) blockBehavior->BallName = props["BallName"];

	return blockBehavior;
}

REGISTER_COMPONENT(BlockBehavior);