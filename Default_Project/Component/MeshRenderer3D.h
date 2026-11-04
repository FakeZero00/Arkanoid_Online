#pragma once
#include "Component.h"
#include <vector>

class Mesh;
class Material;

class MeshRenderer3D : public Component {
public:
	bool isValid = true;
	bool isWireframe = false;

	Mesh* mesh;
	vector<Material*> materials;

	MeshRenderer3D(Mesh* mesh, Material* material);

	void Render(Camera* activeCamera) override;

	static Component* CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects);
};