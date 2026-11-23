#include "MeshRenderer2DLagacy.h"
#include "Transform.h"
#include "Object.h"
#include <gl/glew.h>

MeshRenderer2DLagacy::MeshRenderer2DLagacy() {
	//초기 위치 설정
	minPos.x = 0.0f;
	minPos.y = 0.0f;
	maxPos.x = 1.0f;
	maxPos.y = 1.0f;

	//초기 크기 설정
	width = (maxPos.x - minPos.x);
	height = (maxPos.y - minPos.y);

	Expose("isOutline", &isOutline);
}

MeshRenderer2DLagacy::MeshRenderer2DLagacy(float width, float height, Color color) {
	//크기 설정
	this->width = width;
	this->height = height;
	this->color = color;
}

void  MeshRenderer2DLagacy::SetSize(float width, float height) {
	this->width = width;
	this->height = height;
}

void MeshRenderer2DLagacy::Render() {
	Transform* transform = gameObject->GetComponent<Transform>();
	if (transform == nullptr) return;

	float scaledWidth = width * transform->scale.x;
	float scaledHeight = height * transform->scale.y;

	//꼭짓점 좌표 계산
	minPos.x = transform->worldPosition.x - scaledWidth / 2.0f;
	minPos.y = transform->worldPosition.y - scaledHeight / 2.0f;
	maxPos.x = transform->worldPosition.x + scaledWidth / 2.0f;
	maxPos.y = transform->worldPosition.y + scaledHeight / 2.0f;
	
	//외곽선 그리기
	if (isOutline) {
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glRectf(minPos.x - 0.01f, minPos.y - 0.01f, maxPos.x + 0.01f, maxPos.y + 0.01f);
	}

	//사각형 그리기
	glColor4f(color.r, color.g, color.b, color.a);
	glRectf(minPos.x, minPos.y, maxPos.x, maxPos.y);
}