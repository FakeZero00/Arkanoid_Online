#include "Scene.h"

Object* Scene::Instantiate(AppContext& ctx, string objName) {
	auto newObj = make_unique<Object>(ctx, objName);
	Object* ptr = newObj.get();

	ctx.Hierarchy.push_back(move(newObj));
	return ptr;
}

Object* Scene::FindObject(AppContext& ctx, string objName) {
	for (auto& obj : ctx.Hierarchy) {
		if (obj->name == objName) {
			return obj.get();
		}
	}
	return nullptr;
}