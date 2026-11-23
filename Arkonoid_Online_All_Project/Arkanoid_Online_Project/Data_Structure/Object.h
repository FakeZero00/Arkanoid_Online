#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Component.h"
#include "AppContext.h"
using namespace std;

class Object {
public:
	static unsigned int objCounter; //오브젝트 고유 ID를 위한 카운터
	unsigned int id;
	bool isValid = true;
	bool isStarted = false;

	string name = "GameObject";
	AppContext& ctx;
	vector<unique_ptr<Component>> components;

	Object(AppContext& context, string str);
	Object* Instantiate(string name);

	template <typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		T* newComponent = new T(forward<Args>(args)...);

		//본 오브젝트를 부모 오브젝트로 설정
		newComponent->gameObject = this;
		components.push_back(unique_ptr<Component>(newComponent));

		//추가될 때, 컴포넌트의 Awake 호출
		newComponent->Awake();
		return newComponent;
	}

	template <typename T>
	T* GetComponent() {
		for (auto& comp : components) {
			T* target = dynamic_cast<T*>(comp.get());
			if (target != nullptr) return target;
		}

		return nullptr;
	}

	int Get_ID() const { return id; }

	void Awake();
	void Start();
	void Update(double deltaTime);
	void Render(Camera* activeCamera);
	void Destroy();
};