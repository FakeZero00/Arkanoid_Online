#pragma once
#include <vector>
#include <memory>
#include <gl/glm/glm.hpp>
#include "InputManager.h"
#include "Light.h"
#include "Collider.h"
using namespace glm;

class Object;
class Camera;

struct AppContext {
	vec4 bgColor = { 0.0f, 0.0f, 0.0f, 0.0f };				//배경색 초기화
	double time = 0.0;
	double deltaTime = 0.0;
	InputManager inputManager;								//입력 관리 객체

	vector<unique_ptr<Object>> Hierarchy;					//게임 오브젝트 계층 구조를 저장하는 벡터
	vector<unique_ptr<Object>> pendingHierarchy;			//새로 생성된 게임 오브젝트를 임시로 저장하는 벡터
	vector<Object*> pendingDestroyObjects;					//제거할 오브젝트를 저장하는 벡터

	vector<Collider*> CollisionObjects;						//충돌 감지를 위한 오브젝트 벡터
	vector<Collider*> pendingCollisionObjects;				//새로 생성된 충돌 감지 오브젝트를 임시로 저장하는 벡터
	vector<Collider*> pendingDestroyCollisionObjects;		//제거할 충돌 감지 오브젝트를 저장하는 벡터

	vector<Camera*> cameras;								//씬에 존재하는 카메라들을 저장하는 벡터
	vector<Camera*> pendingCameras;							//제거할 카메라를 저장하는 벡터

	vector<Light*> lights;									//씬에 존재하는 라이트들을 저장하는 벡터
};