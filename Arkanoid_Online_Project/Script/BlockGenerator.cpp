#include "BlockGenerator.h"
#include "Object.h"

#include "BlockBehavior.h"

extern map<string, Mesh*> meshChache;
extern map<string, Material*> materialChache;

void BlockGenerator::Start() {
	if (stage == 1) {
		string colors[6] = { "Silver", "Red", "Blue", "Orange", "Magenta", "Green" };
		for (int row = 0; row < 6; ++row) {
			for (int i = 0; i < 11; ++i) {
				Object* block = gameObject->Instantiate("Block" + to_string(i));
				Transform* blockTr = block->GetComponent<Transform>();
				block->AddComponent<MeshRenderer3D>(meshChache["Block"], materialChache[colors[row]]);
				block->AddComponent<BoxCollider>(vec3{ 0.0f, 0.0f, 0.0f }, vec3{ 1.0f, 1.0f, 1.0f })->isDebug = true;
				block->AddComponent<BlockBehavior>();

				float xPos = -14.14f + (2.828f * (float)i);
				float yPos = 31.87f - (1.6f * (float)row);
				blockTr->SetLocalPosition(xPos, yPos, 0.0f);
			}
		}
	}
}

Component* BlockGenerator::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	BlockGenerator* blockGenerator = obj->AddComponent<BlockGenerator>();
	
	if (props.contains("stage")) blockGenerator->stage = props["stage"].get<int>();

	return blockGenerator;
}

REGISTER_COMPONENT(BlockGenerator);