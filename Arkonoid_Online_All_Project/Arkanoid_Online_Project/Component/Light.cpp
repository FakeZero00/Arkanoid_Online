#include "Light.h"
#include "Object.h"

void Light::Update(float deltaTime) {
	// Update logic for the light component can be implemented here
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

	return light;
}

REGISTER_COMPONENT(Light);