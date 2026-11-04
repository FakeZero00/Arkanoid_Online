#pragma once
#include "../Common.h"
#include <json.hpp>
#include "gl/glm/glm.hpp"
using namespace std;
using namespace glm;
using json = nlohmann::json;

class SceneLoader {
public:
	ScreenSize screenSize;
	map<string, Mesh*> meshChache;
	map<string, Material*> materialChache;

	void LoadScene(AppContext& ctx, const string& sceneFile);

private:
	void AttachComponent(Object* obj, const json& compData, map<string, Object*> spawnedObjects);
	Object* Instantiate(AppContext& ctx, string objName);
};