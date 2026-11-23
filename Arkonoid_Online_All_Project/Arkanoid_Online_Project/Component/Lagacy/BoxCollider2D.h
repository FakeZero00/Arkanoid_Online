#pragma once
#include <vector>
#include "Component.h"
#include "Transform.h"
#include "Position2D.h"

class BoxCollider2D : public Component {
public:
	Transform* transform = nullptr;

	Position2D minPos;
	Position2D maxPos;

	float width;
	float height;

	vector<BoxCollider2D*> collidingColliders;

	BoxCollider2D(float width, float height);

	void SetSize(float width, float height);
	void RecalculateCollision();
	bool Contains(float x, float y);

	void Awake() override;
	void Update(float deltaTime) override;
};