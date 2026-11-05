#include "Camera.h"
#include "Object.h"
#include "Transform.h"
#include "ScreenSize.h"
#include <gl/glm/gtc/matrix_transform.hpp>

extern ScreenSize screenSize;

Camera::Camera(float width, float height) : width(width), height(height) {
	aspectRatio = width / height;

	Expose("fov", &fov);
	Expose("aspectRatio", &aspectRatio);
	Expose("nearPlane", &nearPlane);
	Expose("farPlane", &farPlane);
}

void Camera::Start() {
	//카메라를 AppContext에 등록
	gameObject->ctx.pendingCameras.push_back(this);
}

mat4 Camera::GetViewMatrix() {
	Transform* tr = gameObject->GetComponent<Transform>();

	vec3 eye = tr->worldPosition;

	//월드 회전이 적용된 forward와 up 벡터를 계산
	mat3 worldRot = mat3(tr->worldMatrix);
	vec3 worldForward = glm::normalize(worldRot * vec3(0.0f, 0.0f, 1.0f));
	vec3 worldUp = glm::normalize(worldRot * vec3(0.0f, 1.0f, 0.0f));

	return lookAt(eye, eye + worldForward, worldUp);
}

mat4 Camera::GetProjectionMatrix() {
	if (isOrthographic) {
		float halfWidth = width / 2.0f / 100.0f;
		float halfHeight = height / 2.0f / 100.0f;
		return ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
	}
	else return perspective(radians(fov), aspectRatio, nearPlane, farPlane);
}

Component* Camera::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	int width = props.contains("width") ? static_cast<int>(props["width"]) : screenSize.width;
	int height = props.contains("height") ? static_cast<int>(props["height"]) : screenSize.height;

	Camera* cam = obj->AddComponent<Camera>(width, height);
	if (props.contains("fov")) cam->fov = props["fov"];
	if (props.contains("nearPlane")) cam->nearPlane = props["nearPlane"];
	if (props.contains("farPlane")) cam->farPlane = props["farPlane"];
	if (props.contains("isOrthographic")) cam->isOrthographic = props["isOrthographic"];
	if (props.contains("priority")) cam->priority = props["priority"];
	if (props.contains("backgroundColor")) {
		auto colorArray = props["backgroundColor"];
		if (colorArray.is_array() && colorArray.size() == 4) {
			cam->backgroundColor = vec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
		}
	}
	if (props.contains("viewportRect")) {
		auto rectArray = props["viewportRect"];
		if (rectArray.is_array() && rectArray.size() == 4) {
			cam->viewportRect = vec4(rectArray[0], rectArray[1], rectArray[2], rectArray[3]);
		}
	}
	return cam;
}

REGISTER_COMPONENT(Camera)