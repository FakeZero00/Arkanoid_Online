#include "BousController.h"
#include "Object.h"

void BousController::Start()
{
	tr = gameObject->GetComponent<Transform>();
	inputManager = &gameObject->ctx.inputManager;

	ballController = ball->GetComponent<BallController>();
}

void BousController::Update(float deltaTime)
{
	if (inputManager->GetKeyPressed(GLFW_KEY_LEFT)) {
		if(canMoveLeft) tr->Translate(-speed * deltaTime, 0.0f, 0.0f);
	}
	else if(inputManager->GetKeyPressed(GLFW_KEY_RIGHT)) {
		if (canMoveRight) tr->Translate(speed * deltaTime, 0.0f, 0.0f);
	}
}

void BousController::OnTriggerEnter(Object* other)
{
	if (other->name == "WallCol.L") {
		canMoveLeft = false;
		ballController->canMoveLeft = false;
	}
	else if (other->name == "WallCol.R") {
		canMoveRight = false;
		ballController->canMoveRight = false;
	}
}

void BousController::OnTriggerExit(Object* other)
{
	if (other->name == "WallCol.L") {
		canMoveLeft = true;
		ballController->canMoveLeft = true;
	}
	else if (other->name == "WallCol.R") {
		canMoveRight = true;
		ballController->canMoveRight = true;
	}
}

Component* BousController::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	BousController* bousController = obj->AddComponent<BousController>();

	if (props.contains("ball")) bousController->ball = spawnedObjects.at(props["ball"]);

	return bousController;
}

REGISTER_COMPONENT(BousController);