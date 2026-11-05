#include "RigidBody.h"
#include "Object.h"

void RigidBody::AddForce(vec3 force, ForceMode mode) {
	if (mode == Force) {
		forceAccumulator += force;
	}
	else if (mode == Impulse) {
		velocity += force / mass;
	}
}

void RigidBody::Start() {
	tr = gameObject->GetComponent<Transform>();
}

void RigidBody::Update(float deltaTime) {
	if (not tr) return;
	if (!isKinematic) {
		if (useGravity) {
			forceAccumulator += gravity * mass;
		}

		//알짜힘을 질량으로 나누어 가속도 계산
		vec3 acceleration = forceAccumulator / mass;

		//가속도를 속도에 적용
		velocity += acceleration * deltaTime;
		forceAccumulator = vec3{ 0.0f }; // 초기화

		//속도를 실제 위치에 적용
		tr->Translate(velocity * deltaTime, true);
	}
}

Component* RigidBody::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects)
{
	RigidBody* rigidBody = obj->AddComponent<RigidBody>();

	if (props.contains("mass")) rigidBody->mass = props["mass"];
	if (props.contains("useGravity")) rigidBody->useGravity = props["useGravity"];
	if (props.contains("isKinematic")) rigidBody->isKinematic = props["isKinematic"];

	return rigidBody;
}

REGISTER_COMPONENT(RigidBody);