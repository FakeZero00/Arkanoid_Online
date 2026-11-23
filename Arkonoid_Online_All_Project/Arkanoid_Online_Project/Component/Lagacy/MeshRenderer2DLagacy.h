#pragma once
#include "Component.h"
#include "Color.h"
#include "Position2D.h"

class MeshRenderer2DLagacy : public Component {
public:
	Position2D minPos;
	Position2D maxPos;

	float width;
	float height;
	Color color = { 1.0f, 1.0f, 1.0f, 1.0f }; //기본 색상 흰색

	bool isOutline = false;

	MeshRenderer2DLagacy();
	MeshRenderer2DLagacy(float width, float height, Color color);

	void SetSize(float width, float height);

	void Render() override;
};