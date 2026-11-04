#pragma once
#include "../Common.h"
#include "ScreenSize.h"

class Scene {
public:
	string name;

	ScreenSize screenSize{ 1600, 900 };

	Object* Instantiate(AppContext& ctx, string objName);
	Object* FindObject(AppContext& ctx, string objName);

	virtual void LoadScene(AppContext& ctx) {}
};