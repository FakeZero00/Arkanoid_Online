#include "BousController.h"
#include "Object.h"

void BousController::Start()
{
	tr = gameObject->GetComponent<Transform>();
	inputManager = &gameObject->ctx.inputManager;
}

void BousController::Update(float deltaTime)
{
	if (inputManager->GetKeyPressed(GLFW_KEY_LEFT)) {
		tr->Translate(-speed * deltaTime, 0.0f, 0.0f);
	}
	else if(inputManager->GetKeyPressed(GLFW_KEY_RIGHT)) {
		tr->Translate(speed * deltaTime, 0.0f, 0.0f);
	}
}

Component* BousController::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	BousController* bousController = obj->AddComponent<BousController>();

	return bousController;
}

REGISTER_COMPONENT(BousController);