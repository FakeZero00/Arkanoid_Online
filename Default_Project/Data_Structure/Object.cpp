#include "Object.h"
#include <string>
#include "Transform.h"
#include "AppContext.h"

Object::Object(AppContext& context, string str) : ctx(context), name(str) {
	//Transform 컴포넌트 생성 및 추가
	auto transform = make_unique<Transform>();
	transform->gameObject = this; //Transform의 gameObject를 현재 Object로 설정

	components.push_back(move(transform)); //컴포넌트 벡터에 추가
}

Object* Object::Instantiate(string name) {
	auto newObj = make_unique<Object>(ctx, name);
	Object* ptr = newObj.get();

	ctx.pendingHierarchy.push_back(move(newObj));
	Transform* newObjTr = ptr->GetComponent<Transform>();
	newObjTr->SetParent(this->GetComponent<Transform>()); //부모 Transform 설정
	return ptr;
}

void Object::Awake() {
	for (auto& comp : components) {
		comp->Awake();
	}
}

void Object::Start() {
	for (auto& comp : components) {
		comp->hasStarted = true;
		comp->Start();
	}
}

void Object::Update(double deltaTime) {
	for (auto& comp : components) {
		if (!comp->hasStarted) {
			comp->Start();
		}

		comp->Update(static_cast<float>(deltaTime));
	}
}

void Object::Render(Camera* activeCamera) {
	for (auto& comp : components) {
		comp->Render(activeCamera);
	}
}

void Object::Destroy() {
	ctx.pendingDestroyObjects.push_back(this);
}