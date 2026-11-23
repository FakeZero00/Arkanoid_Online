#include "BoxCollider2D.h"
#include "Transform.h"
#include "Object.h"

BoxCollider2D::BoxCollider2D(float width, float height) : width(width), height(height) {
	//임시 초기화
	minPos.x = 0.0f;
	minPos.y = 0.0f;
	maxPos.x = 0.0f;
	maxPos.y = 0.0f;
}

void BoxCollider2D::SetSize(float width, float height) {
	this->width = width;
	this->height = height;
}

bool BoxCollider2D::Contains(float x, float y) {
	//AABB 충돌 감지: 점이 박스 안에 있는지 확인
	/*cout << "Checking if point (" << x << ", " << y << ") is within BoxCollider2D bounds: "
		<< "MinPos(" << minPos.x << ", " << minPos.y << "), "
		<< "MaxPos(" << maxPos.x << ", " << maxPos.y << ")" << endl;*/
	return (x >= minPos.x && x <= maxPos.x && y >= minPos.y && y <= maxPos.y);
}

void BoxCollider2D::RecalculateCollision() {
	transform = gameObject->GetComponent<Transform>();

	float scaledWidth = width * transform->scale.x;
	float scaledHeight = height * transform->scale.y;

	minPos.x = transform->worldPosition.x - scaledWidth / 2.0f;
	minPos.y = transform->worldPosition.y - scaledHeight / 2.0f;
	maxPos.x = transform->worldPosition.x + scaledWidth / 2.0f;
	maxPos.y = transform->worldPosition.y + scaledHeight / 2.0f;
}

void BoxCollider2D::Awake() {
	transform = gameObject->GetComponent<Transform>();
	if (transform == nullptr) {
		//Transform 컴포넌트가 없으면 경고 메시지 출력
		std::cerr << "Warning: BoxCollider2D requires a Transform component." << std::endl;
		return;
	}

	float scaledWidth = width * transform->scale.x;
	float scaledHeight = height * transform->scale.y;

	minPos.x = transform->worldPosition.x - scaledWidth / 2.0f;
	minPos.y = transform->worldPosition.y - scaledHeight / 2.0f;
	maxPos.x = transform->worldPosition.x + scaledWidth / 2.0f;
	maxPos.y = transform->worldPosition.y + scaledHeight / 2.0f;

	gameObject->ctx.pendingCollisionObjects.push_back(this);
}

void BoxCollider2D::Update(float deltaTime) {
	RecalculateCollision();
}