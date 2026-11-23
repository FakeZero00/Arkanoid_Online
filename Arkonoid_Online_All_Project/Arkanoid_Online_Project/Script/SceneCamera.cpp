#include "SceneCamera.h"

#include "Object.h"
#include "Transform.h"
#include "InputManager.h"

void SceneCamera::Awake() {
	Transform* tr = gameObject->GetComponent<Transform>();
	pitch = tr->EulerAngles.x;
	yaw = tr->EulerAngles.y;
}

void SceneCamera::Update(float deltaTime) {
	InputManager inputManager = gameObject->ctx.inputManager;
	Transform* tr = gameObject->GetComponent<Transform>();

	if (inputManager.GetKeyDown(GLFW_MOUSE_BUTTON_RIGHT)) {
		lastMouseX = inputManager.GetMouseX();
		lastMouseY = inputManager.GetMouseY();
	}

	if (inputManager.GetKeyPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
		float currentMouseX = inputManager.GetMouseX();
		float currentMouseY = inputManager.GetMouseY();

		float deltaX = currentMouseX - lastMouseX;
		float deltaY = currentMouseY - lastMouseY;

		lastMouseX = currentMouseX;
		lastMouseY = currentMouseY;

		yaw -= deltaX * sensitivity;
		pitch -= deltaY * sensitivity;

		// 화면이 뒤집히는 현상(Gimbal Lock) 방지를 위해 상하(Pitch) 각도 제한
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		tr->SetLocalRotation(pitch, yaw, 0.0f);
	}

	float speed = walkSpeed;

	if (inputManager.GetKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
		speed = dashSpeed;
	}
	if (inputManager.GetKeyUp(GLFW_KEY_LEFT_SHIFT)) {
		speed = walkSpeed;
	}

	if (inputManager.GetKeyPressed(GLFW_KEY_W)) {
		tr->Translate(0.0f, 0.0f, speed * deltaTime);
	}
	if (inputManager.GetKeyPressed(GLFW_KEY_S)) {
		tr->Translate(0.0f, 0.0f, -speed * deltaTime);
	}
	if (inputManager.GetKeyPressed(GLFW_KEY_A)) {
		tr->Translate(speed * deltaTime, 0.0f, 0.0f);
	}
	if (inputManager.GetKeyPressed(GLFW_KEY_D)) {
		tr->Translate(-speed * deltaTime, 0.0f, 0.0f);
	}
	if (inputManager.GetKeyPressed(GLFW_KEY_Q)) {
		tr->Translate(0.0f, -speed * deltaTime, 0.0f, true);
	}
	if (inputManager.GetKeyPressed(GLFW_KEY_E)) {
		tr->Translate(0.0f, speed * deltaTime, 0.0f, true);
	}
}

Component* SceneCamera::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	SceneCamera* sceneCamera = obj->AddComponent<SceneCamera>();
	return sceneCamera;
}

REGISTER_COMPONENT(SceneCamera);