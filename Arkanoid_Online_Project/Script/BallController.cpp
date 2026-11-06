#include "BallController.h"
#include "Object.h"

void BallController::Start()
{
	tr = gameObject->GetComponent<Transform>();
	inputManager = &gameObject->ctx.inputManager;
}

void BallController::Update(float deltaTime)
{
	if (inputManager->GetKeyPressed(GLFW_KEY_LEFT)) {
		tr->Translate(-speed * deltaTime, 0.0f, 0.0f);
	}
	else if(inputManager->GetKeyPressed(GLFW_KEY_RIGHT)) {
		tr->Translate(speed * deltaTime, 0.0f, 0.0f);
	}
}

Component* BallController::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	BallController* ballController = obj->AddComponent<BallController>();

	return ballController;
}

REGISTER_COMPONENT(BallController);