#pragma once
#include "Component.h"
#include "LightType.h"
#include "gl/glm/glm.hpp"
using namespace glm;

class Light : public Component {
public:
	LightType lightType = LightType::Directional;
	vec4 lightColor = vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
	float intensity = 1.0f;

	// 거리 감쇠(Attenuation) 파라미터
	float radius = 10.0f;        // 빛이 영향을 미치는 최대 거리

	// 스팟 라이트 원뿔 각도 (단위: Degree)
	float cutOff = 12.5f;        // 내부 밝은 영역 각도
	float outerCutOff = 15.0f;   // 빛이 완전히 사라지는 외부 영역 각도

	Light() = default;
	void Awake() override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};