#include "Light.h"
#include "Object.h"

void Light::Awake() {
	gameObject->ctx.lights.push_back(this);
}

Component* Light::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	Light* light = obj->AddComponent<Light>();

	if (props.contains("lightType")) {
		std::string typeStr = props["lightType"];
		if (typeStr == "Directional") light->lightType = LightType::Directional;
		else if (typeStr == "Point") light->lightType = LightType::Point;
		else if (typeStr == "Spot") light->lightType = LightType::Spot;
	}
	if (props.contains("lightColor")) {
		auto colorArray = props["lightColor"];
		if (colorArray.is_array() && colorArray.size() == 4) {
			light->lightColor = glm::vec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
		}
	}
	if (props.contains("intensity")) {
		light->intensity = props["intensity"];
	}

	// Spotlight 및 감쇠 속성 로드
	if (props.contains("radius")) light->radius = props["constant"];

	if (props.contains("cutOff")) light->cutOff = props["cutOff"];
	if (props.contains("outerCutOff")) light->outerCutOff = props["outerCutOff"];

	return light;
}

REGISTER_COMPONENT(Light);