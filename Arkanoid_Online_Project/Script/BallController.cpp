#include "BallController.h"
#include "Object.h"

void BallController::Start()
{
	tr = gameObject->GetComponent<Transform>();
	inputManager = &gameObject->ctx.inputManager;
}

void BallController::Update(float deltaTime)
{
	if (isCatched) {
		if (inputManager->GetKeyPressed(GLFW_KEY_LEFT)) {
			tr->Translate(-speed * deltaTime, 0.0f, 0.0f);
			velocity.x = -speed;
		}
		else if (inputManager->GetKeyPressed(GLFW_KEY_RIGHT)) {
			tr->Translate(speed * deltaTime, 0.0f, 0.0f);
			velocity.x = speed;
		}
		if (inputManager->GetKeyDown(GLFW_KEY_X)) {
			isCatched = false;
			velocity.y = speed;
		}
	}
	else {
		tr->Translate(velocity.x * deltaTime, velocity.y * deltaTime, 0.0f);
	}
}

void BallController::OnTriggerEnter(Object* other) {
	Transform* otherTr = other->GetComponent<Transform>();
	vec3 dir = otherTr->position - tr->position;

	//충돌시 절대값으로 반사하는 것은 공과 2개 이상의 오브젝트가 거의 동시에 충돌했을 때 공이 튕기는 방향이 이상하게 나오는 문제를 해결하기 위함
	if (other->name == "WallCol.L") velocity.x = std::abs(velocity.x); 
	else if (other->name == "WallCol.R") velocity.x = -std::abs(velocity.x);
	else if (other->name == "WallCol.T") velocity.y = -std::abs(velocity.y); // 천장 -> 아래(-)로
	else if (other->name == "Bous") {
		if (dir.x >= 0) velocity.x = -std::abs(velocity.x);
		else velocity.x = std::abs(velocity.x);
		velocity.y = -velocity.y;
	}
	//벽돌에 충돌했을 경우
	else {
		//공으로 부터 충돌한 오브젝트까지의 방향벡터의 x, y 중 큰 값에 따라 반사 방향을 결정
		cout << other->name << endl;
		if (std::abs(dir.x) >= std::abs(dir.y)) {
			if (dir.x >= 0.0) velocity.x = -std::abs(velocity.x);
			else velocity.x = std::abs(velocity.x);
		}
		else {
			if (dir.y >= 0.0) velocity.y = -std::abs(velocity.y);
			else velocity.y = std::abs(velocity.y);
		}
	}
}

Component* BallController::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	BallController* ballController = obj->AddComponent<BallController>();

	return ballController;
}

REGISTER_COMPONENT(BallController);