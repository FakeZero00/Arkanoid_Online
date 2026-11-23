#include "Transform.h"
#include "Collider.h"
#include "Object.h"

Transform::Transform() {
	//초기 위치 설정
	position = vec3{ 0.0f };
	rotation = quat{ 1.0f, 0.0f, 0.0f, 0.0f }; //단위 쿼터니언
	scale = vec3{ 1.0f };

	worldPosition = vec3{ 0.0f };
	worldMatrix = mat4{ 1.0f }; //단위 행렬
}

void Transform::SetLocalPosition(float x, float y, float z) {
	position = vec3{ x, y, z };

	CalculateWorldMatrix();
}

void  Transform::SetLocalPosition(vec3 newPos) {
	position = newPos;
	CalculateWorldMatrix();
}

void Transform::Translate(float x, float y, float z, bool isWorld) {
	if (isWorld) position += vec3{ x, y, z };
	else {
		position += right * x;
		position += up * y;
		position += forward * z;
	}

	CalculateWorldMatrix();
}

void Transform::Translate(vec3 delta, bool isWorld) {
	if (isWorld) position += delta;

	else {
		position += right * delta.x;
		position += up * delta.y;
		position += forward * delta.z;
	}

	CalculateWorldMatrix();
}

void Transform::SetLocalRotation(float x, float y, float z) {
	vec3 eulerRadians = vec3{ radians(x), radians(y), radians(z) };
	rotation = quat(eulerRadians);
	EulerAngles = vec3{ x, y, z };

	CalculateWorldMatrix();
}

void Transform::SetLocalRotation(quat newRot) {
	rotation = newRot;
	//쿼터니언을 오일러 각도로 변환
	vec3 eulerRadians = eulerAngles(rotation);
	EulerAngles = vec3{ degrees(eulerRadians.x), degrees(eulerRadians.y), degrees(eulerRadians.z) };

	CalculateWorldMatrix();
}

void Transform::Rotate(float x, float y, float z, bool isWorld) {
	vec3 eulerRadians = vec3{ radians(x), radians(y), radians(z) };
	quat deltaRotation = quat(eulerRadians);
	EulerAngles += vec3{ x, y, z };

	if (isWorld) rotation = normalize(deltaRotation * rotation); //델타 회전을 현재 회전에 곱함
	else rotation = normalize(rotation * deltaRotation); //현재 회전에 델타 회전을 곱함

	CalculateWorldMatrix();
}

void Transform::SetLocalScale(float x, float y, float z) {
	scale = vec3{ x, y, z };

	CalculateWorldMatrix();
}

void Transform::CalculateWorldMatrix() {
	//로컬 행렬 계산 (SRT 순서: Scale -> Rotate -> Translate)
	//OpenGL에서는 열 기준의 행렬을 사용하므로, 행렬 곱셈 순서가 반대임에 주의
	mat4 localMatrix = mat4{ 1.0f };
	localMatrix = translate(localMatrix, position);
	localMatrix *= mat4_cast(rotation);
	localMatrix = glm::scale(localMatrix, scale);

	//부모가 있으면 부모의 월드 행렬을 곱함
	if (parent != NULL) worldMatrix = parent->worldMatrix * localMatrix;
	else worldMatrix = localMatrix;

	//편의성을 위해 worldPosition 계산
	worldPosition = vec3(worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2]);

	//자식들에게도 월드 행렬 계산
	for (Transform* child : children) {
		child->CalculateWorldMatrix();
	}

	if (gameObject) {
		Collider* col = gameObject->GetComponent<Collider>();
		if (col) col->RecalculateCollision();
	}

	//편의성을 위해 right, up, forward 벡터 계산
	right = normalize(rotation * vec3{ 1.0f, 0.0f, 0.0f });
	up = normalize(rotation * vec3{ 0.0f, 1.0f, 0.0f });
	forward = normalize(rotation * vec3{ 0.0f, 0.0f, 1.0f });
}

void Transform::SetParent(Transform* newParent) {
	parent = newParent;
	if (parent != nullptr) {
		//새로운 부모의 자식 벡터에 현재 오브젝트 추가
		parent->children.push_back(this);
	}

	//부모가 바뀌었으므로 월드 행렬 재계산
	CalculateWorldMatrix();
}