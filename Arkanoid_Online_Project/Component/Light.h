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

	void Update(float deltaTime) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};