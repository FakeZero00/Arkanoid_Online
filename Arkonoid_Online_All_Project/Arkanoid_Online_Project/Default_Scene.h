#pragma once
#include "Scene.h"

class Default_Scene : public Scene {
public:
	Default_Scene(ScreenSize screenSize);

	void LoadScene(AppContext& ctx) override;
	void DrawAxis(AppContext& ctx);
};