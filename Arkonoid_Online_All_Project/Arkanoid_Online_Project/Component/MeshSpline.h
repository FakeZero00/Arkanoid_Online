#pragma once
#include "Component.h"
#include <string>
#include <vector>
#include <gl/glm/glm.hpp>
#include <gl/glew.h>
#include <gl/glm/gtc/type_ptr.hpp>
using namespace std;
using namespace glm;

class MeshSpline : public Component {
public:
	vector<vec3> controlPoints;
	vec4 color;
	float lineWidth = 2.0f;

	GLuint VAO = 0, VBO = 0;

	MeshSpline(string& pointObj, vec4 color = vec4{ 0.0f, 1.0f, 0.0f, 1.0f }, float lineWidth = 2.0f);

	void InitRender();
	void SetColor(vec4 newColor);

	void Render(Camera* activeCamera) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};