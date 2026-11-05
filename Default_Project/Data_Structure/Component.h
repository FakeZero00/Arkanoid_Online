#pragma once
#include <map>
#include <string>
#include <iostream>
#include <json.hpp>
using namespace std;
using json = nlohmann::json;

//전방선언
class Object;
class Camera;

enum class PropertyType {
	Float,
	Int,
	String,
	Object,
	ComponentPtr
};

struct Property {
	PropertyType type;
	void* dataPtr;
};

class Component {
public:
	//적용되어 있는 Object를 가리키는 포인터
	Object* gameObject = nullptr;
	bool hasStarted = false;

	//변수들을 저장하는 맵
	map<string, Property> properties;

	//소멸자
	virtual ~Component() = default;

	//라이프 사이클 함수
	virtual void Awake() {}
	virtual void Start() {}
	virtual void Update(float deltaTime) {}
	virtual void Render(Camera* activeCamera) {}

	//콜라이더 충돌 처리 함수
	virtual void OnTriggerEnter(Object* other) {}
	virtual void OnTriggerStay(Object* other) {}
	virtual void OnTriggerExit(Object* other) {}

	//아웃렛 접속 함수
	template <typename T>
	void Expose(string name, T* varAddr) {
		PropertyType pType;

		if constexpr (is_same_v<T, float>) pType = PropertyType::Float;
		else if constexpr (is_same_v<T, int>) pType = PropertyType::Int;
		else if constexpr (is_same_v<T, string>) pType = PropertyType::String;
		else if constexpr (is_same_v<T, Object>) pType = PropertyType::Object;
		else pType = PropertyType::ComponentPtr; //기타 타입은 ComponentPtr로 처리

		properties[name] = { pType, static_cast<void*>(varAddr) };
	}

	template <typename T>
	void SetPropertyValue(string name, T value) {
		if (properties.find(name) == properties.end()) {
			cout << "Property " << name << " not found!" << endl;
		}

		T* ptr = static_cast<T*>(properties[name].dataPtr);
		*ptr = value;
	}
};

// 오브젝트, json을 받아서 컴포넌트를 생성하는 함수 타입 정의
using ComponentCreator = std::function<Component* (Object*, const json&, const map<string, Object*>&)>;

class ComponentRegistry {
public:
	//전역 컴포넌트 등록 레지스트리
	static map<string, ComponentCreator>& GetRegistry() {
		static map<string, ComponentCreator> registry;
		return registry;
	}

	//컴포넌트 등록 함수
	static void Register(const string& typeName, ComponentCreator creator) {
		GetRegistry()[typeName] = creator;
	}
};

#define REGISTER_COMPONENT(TYPE) \
	static bool registered_##TYPE = []() { \
		ComponentRegistry::Register(#TYPE, &TYPE::CreateFromJson); \
		return true; \
	}();