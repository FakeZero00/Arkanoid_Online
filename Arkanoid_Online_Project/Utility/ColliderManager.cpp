#include "ColliderManager.h"
#include <vector>
#include <algorithm>
#include "Object.h"
#include "Component.h"
#include "Collider.h"
#include "RigidBody.h"
#include <iostream>
#include <cmath>
using namespace std;

ColliderManager::ColliderManager(AppContext& ctx) : ctx(ctx) {}

bool ColliderManager::CheckCollision(Collider* colA, Collider* colB) {
	//1. Box vs Box 충돌 체크
	if (colA->type == ColliderType::Box && colB->type == ColliderType::Box) {
		return CheckBoxBox(static_cast<BoxCollider*>(colA), static_cast<BoxCollider*>(colB));
	}

	//2. Sphere vs Sphere 충돌 체크
	else if (colA->type == ColliderType::Sphere && colB->type == ColliderType::Sphere) {
		return CheckSphereSphere(static_cast<SphereCollider*>(colA), static_cast<SphereCollider*>(colB));
	}

	//3. Box vs Sphere 충돌 체크
	else if (colA->type == ColliderType::Box && colB->type == ColliderType::Sphere) {
		return CheckBoxSphere(static_cast<BoxCollider*>(colA), static_cast<SphereCollider*>(colB));
	}
	else if (colA->type == ColliderType::Sphere && colB->type == ColliderType::Box) {
		return CheckBoxSphere(static_cast<BoxCollider*>(colB), static_cast<SphereCollider*>(colA));
	}

	//4. 그 외의 경우
	else return false;
}

bool ColliderManager::CheckBoxBox(BoxCollider* colA, BoxCollider* colB) {
	Transform* trA = colA->gameObject->GetComponent<Transform>();
	Transform* trB = colB->gameObject->GetComponent<Transform>();
	
	// 각 박스의 로컬 중심점 및 반-크기(Half-Extents) 구하기
	vec3 localCenterA = (colA->localMin + colA->localMax) * 0.5f;
	vec3 localExtentA = (colA->localMax - colA->localMin) * 0.5f;

	vec3 localCenterB = (colB->localMin + colB->localMax) * 0.5f;
	vec3 localExtentB = (colB->localMax - colB->localMin) * 0.5f;

	// 월드 중심점 계산
	vec3 centerA = vec3(trA->worldMatrix * vec4(localCenterA, 1.0f));
	vec3 centerB = vec3(trB->worldMatrix * vec4(localCenterB, 1.0f));
	vec3 dist = centerB - centerA;	// 두 박스 중심 간의 거리 벡터

	// 각 박스의 축(로컬 축)과 스케일 반영된 반-크기 산출
	vec3 A[3], B[3];
	vec3 a, b;	// 월드 스케일이 적용된 반-크기

	for (int i = 0; i < 3; ++i) {
		vec3 axisA = vec3(trA->worldMatrix[i]);
		a[i] = localExtentA[i] * glm::length(axisA);
		A[i] = glm::normalize(axisA);

		vec3 axisB = vec3(trB->worldMatrix[i]);
		b[i] = localExtentB[i] * glm::length(axisB);
		B[i] = glm::normalize(axisB);
	}

	// 회전 행렬 R_ij = A_i dot B_j 및 절대값 행렬 계산
	const float EPSILON = 1e-5f;	// 작은 값으로 0 근처의 수치 오차 방지
	float R[3][3], AbsR[3][3];

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			R[i][j] = glm::dot(A[i], B[j]);
			AbsR[i][j] = std::abs(R[i][j]) + EPSILON;
		}
	}

	// A 기준 좌표계에서의 중심 간 거리를 투영
	vec3 t = vec3(glm::dot(dist, A[0]), glm::dot(dist, A[1]), glm::dot(dist, A[2]));

	// 15개의 분리축 투영 검사
	// A의 3개 축 검사 (A0, A1, A2)
	for (int i = 0; i < 3; ++i) {
		float ra = a[i];
		float rb = b[0] * AbsR[i][0] + b[1] * AbsR[i][1] + b[2] * AbsR[i][2];
		if (std::abs(t[i]) > ra + rb) return false;
	}

	// B의 3개 축 검사 (B0, B1, B2)
	for (int i = 0; i < 3; ++i) {
		float ra = a[0] * AbsR[0][i] + a[1] * AbsR[1][i] + a[2] * AbsR[2][i];
		float rb = b[i];
		float term = std::abs(glm::dot(dist, B[i]));
		if (term > ra + rb) return false;
	}

	//외적 축 9개 검사
	// A0 x B0
	if (std::abs(t[2] * R[1][0] - t[1] * R[2][0]) > a[1] * AbsR[2][0] + a[2] * AbsR[1][0] + b[1] * AbsR[0][2] + b[2] * AbsR[0][1]) return false;
	// A0 x B1
	if (std::abs(t[2] * R[1][1] - t[1] * R[2][1]) > a[1] * AbsR[2][1] + a[2] * AbsR[1][1] + b[0] * AbsR[0][2] + b[2] * AbsR[0][0]) return false;
	// A0 x B2
	if (std::abs(t[2] * R[1][2] - t[1] * R[2][2]) > a[1] * AbsR[2][2] + a[2] * AbsR[1][2] + b[0] * AbsR[0][1] + b[1] * AbsR[0][0]) return false;

	// A1 x B0
	if (std::abs(t[0] * R[2][0] - t[2] * R[0][0]) > a[0] * AbsR[2][0] + a[2] * AbsR[0][0] + b[1] * AbsR[1][2] + b[2] * AbsR[1][1]) return false;
	// A1 x B1
	if (std::abs(t[0] * R[2][1] - t[2] * R[0][1]) > a[0] * AbsR[2][1] + a[2] * AbsR[0][1] + b[0] * AbsR[1][2] + b[2] * AbsR[1][0]) return false;
	// A1 x B2
	if (std::abs(t[0] * R[2][2] - t[2] * R[0][2]) > a[0] * AbsR[2][2] + a[2] * AbsR[0][2] + b[0] * AbsR[1][1] + b[1] * AbsR[1][0]) return false;

	// A2 x B0
	if (std::abs(t[1] * R[0][0] - t[0] * R[1][0]) > a[0] * AbsR[1][0] + a[1] * AbsR[0][0] + b[1] * AbsR[2][2] + b[2] * AbsR[2][1]) return false;
	// A2 x B1
	if (std::abs(t[1] * R[0][1] - t[0] * R[1][1]) > a[0] * AbsR[1][1] + a[1] * AbsR[0][1] + b[0] * AbsR[2][2] + b[2] * AbsR[2][0]) return false;
	// A2 x B2
	if (std::abs(t[1] * R[0][2] - t[0] * R[1][2]) > a[0] * AbsR[1][2] + a[1] * AbsR[0][2] + b[0] * AbsR[2][1] + b[1] * AbsR[2][0]) return false;

	// 15개 축 중 분리축을 찾지 못했다면 두 OBB는 충돌 상태!
	return true;
}

bool ColliderManager::CheckSphereSphere(SphereCollider* colA, SphereCollider* colB) {
	float dist = glm::distance(colA->worldCenter, colB->worldCenter);
	return dist <= (colA->worldRadius + colB->worldRadius);
}

bool ColliderManager::CheckBoxSphere(BoxCollider* box, SphereCollider* sphere) {
	Transform* boxTr = box->gameObject->GetComponent<Transform>();

	// 구체의 월드 좌표를 박스 오브젝트의 로컬 좌표계로 역변환
	mat4 invBoxMatrix = glm::inverse(boxTr->worldMatrix);
	vec3 localSphereCenter = vec3(invBoxMatrix * vec4(sphere->worldCenter, 1.0f));

	// AABB 박스 안에서 구체의 중심점과 가장 가까운 x,y,z 좌표를 찾음 (Clamp)
	float closestX = glm::clamp(localSphereCenter.x, box->localMin.x, box->localMax.x);
	float closestY = glm::clamp(localSphereCenter.y, box->localMin.y, box->localMax.y);
	float closestZ = glm::clamp(localSphereCenter.z, box->localMin.z, box->localMax.z);

	vec3 localClosestPoint = vec3(closestX, closestY, closestZ);

	// 가장 가까운 로컬 점을 다시 월드 좌표로 복원, 구체 월드 중심과 거리 비교
	vec3 worldClosestPoint = vec3(boxTr->worldMatrix * vec4(localClosestPoint, 1.0f));
	float dist = glm::distance(worldClosestPoint, sphere->worldCenter);

	return dist <= sphere->worldRadius;
}

void ColliderManager::Update() {
	vector<Collider*>& CollisionObjects = ctx.CollisionObjects;

	for (size_t i = 0; i < CollisionObjects.size(); ++i) {
		for (size_t j = i + 1; j < CollisionObjects.size(); ++j) {
			Collider* colA = CollisionObjects[i];
			Collider* colB = CollisionObjects[j];

			if (!colA->gameObject->isValid) continue;
			if (!colB->gameObject->isValid) continue;

			if (colA && colB) {
				bool isColliding = CheckCollision(colA, colB);

				if (isColliding && !colA->isTrigger && !colB->isTrigger) {
					Transform* trA = colA->gameObject->GetComponent<Transform>();
					Transform* trB = colB->gameObject->GetComponent<Transform>();

					//A에서 B를 향하는 중심선 벡터로 Normal 기본값 설정 (A->B 밀어내기 축 방향 보장)
					vec3 centerA = trA->worldPosition;
					vec3 centerB = trA->worldPosition;

					if (colA->type == ColliderType::Box) {
						auto boxA = static_cast<BoxCollider*>(colA);
						vec3 localCenterA = (boxA->localMin + boxA->localMax) * 0.5f;
						centerA = vec3(trA->worldMatrix * vec4(localCenterA, 1.0f));
					}
					else if (colA->type == ColliderType::Sphere) {
						centerA = static_cast<SphereCollider*>(colA)->worldCenter;
					}

					if (colB->type == ColliderType::Box) {
						auto boxB = static_cast<BoxCollider*>(colB);
						vec3 localCenterB = (boxB->localMin + boxB->localMax) * 0.5f;
						centerB = vec3(trB->worldMatrix * vec4(localCenterB, 1.0f));
					}
					else if (colB->type == ColliderType::Sphere) {
						centerB = static_cast<SphereCollider*>(colB)->worldCenter;
					}

					vec3 dir = centerB - centerA;

					//두 오브젝트의 충돌 수직 방향 계산
					vec3 normal = vec3{ 0.0f, 1.0f, 0.0f };
					if (glm::length(dir) > 0.0001f) normal = glm::normalize(dir);

					// 기본 침투 깊이
					float penetration = 0.01f;

					//Sphere vs Sphere 충돌 시, 두 구체 중심점의 거리로 노말 계산
					if(colA->type == ColliderType::Sphere && colB->type == ColliderType::Sphere) {
						auto sphA = static_cast<SphereCollider*>(colA);
						auto sphB = static_cast<SphereCollider*>(colB);

						vec3 sphereDir = sphB->worldCenter - sphA->worldCenter;
						float dist = glm::length(sphereDir);

						if (dist > 0.0001f) {
							normal = sphereDir / dist;
							penetration = (sphA->worldRadius + sphB->worldRadius) - dist;
						}
						else {
							// 예외 처리: 두 구체의 중심이 완전히 똑같은 위치에 겹쳤을 때
							normal = vec3(0.0f, 1.0f, 0.0f); // 임의로 위로 밀어냄
							penetration = sphA->worldRadius + sphB->worldRadius;
						}
					}
					//Box vs Box 충돌 시, Y축 침투량 자동 산출
					else if (colA->type == ColliderType::Box && colB->type == ColliderType::Box) {
						auto boxA = static_cast<BoxCollider*>(colA);
						auto boxB = static_cast<BoxCollider*>(colB);

						// 각 박스의 로컬 반-크기(Half-Extents)
						vec3 localExtA = (boxA->localMax - boxA->localMin) * 0.5f;
						vec3 localExtB = (boxB->localMax - boxB->localMin) * 0.5f;

						// 각 박스의 로컬 축 벡터(정규화)와 스케일 반영된 크기 추출
						vec3 A[3], B[3];
						vec3 a, b;
						for (int k = 0; k < 3; ++k) {
							vec3 axisA = vec3(trA->worldMatrix[k]);
							a[k] = localExtA[k] * glm::length(axisA);
							A[k] = glm::normalize(axisA);

							vec3 axisB = vec3(trB->worldMatrix[k]);
							b[k] = localExtB[k] * glm::length(axisB);
							B[k] = glm::normalize(axisB);
						}

						// 검사할 6개의 주요 표면 축 (A의 3축, B의 3축)
						vec3 axes[6] = { A[0], A[1], A[2], B[0], B[1], B[2] };
						float minOverlap = FLT_MAX;
						vec3 bestAxis = vec3(0.0f, 1.0f, 0.0f);

						for (int k = 0; k < 6; ++k) {
							vec3 axis = axes[k];
							if (glm::length(axis) < 0.0001f) continue;

							// A와 B의 크기를 현재 검사 중인 축(axis)에 투영
							float rA = a[0] * std::abs(glm::dot(A[0], axis)) +
								a[1] * std::abs(glm::dot(A[1], axis)) +
								a[2] * std::abs(glm::dot(A[2], axis));

							float rB = b[0] * std::abs(glm::dot(B[0], axis)) +
								b[1] * std::abs(glm::dot(B[1], axis)) +
								b[2] * std::abs(glm::dot(B[2], axis));

							// 두 박스 중심 간의 거리를 같은 축에 투영
							float projDist = std::abs(glm::dot(dir, axis));
							float overlap = (rA + rB) - projDist;

							// 가장 얕게 겹친 축(최소 침투) 찾기
							if (overlap < minOverlap) {
								minOverlap = overlap;
								bestAxis = axis;
							}
						}

						// 최종 충돌 방향과 밀어낼 깊이 결정
						penetration = minOverlap;
						normal = (glm::dot(dir, bestAxis) > 0.0f) ? bestAxis : -bestAxis;
					}
					else if ((colA->type == ColliderType::Box && colB->type == ColliderType::Sphere) ||
						(colA->type == ColliderType::Sphere && colB->type == ColliderType::Box)) {
						BoxCollider* box = (colA->type == ColliderType::Box) ? static_cast<BoxCollider*>(colA) : static_cast<BoxCollider*>(colB);
						SphereCollider* sphere = (colA->type == ColliderType::Sphere) ? static_cast<SphereCollider*>(colA) : static_cast<SphereCollider*>(colB);
						Transform* boxTr = box->gameObject->GetComponent<Transform>();

						// 구체 중심을 박스의 로컬 좌표계로 변환
						mat4 invBoxMatrix = glm::inverse(boxTr->worldMatrix);
						vec3 localSphereCenter = vec3(invBoxMatrix * vec4(sphere->worldCenter, 1.0f));

						// 구체 중심이 박스 내부에 완전히 들어왔는지 확인
						bool isInside = (localSphereCenter.x > box->localMin.x && localSphereCenter.x < box->localMax.x &&
							localSphereCenter.y > box->localMin.y && localSphereCenter.y < box->localMax.y &&
							localSphereCenter.z > box->localMin.z && localSphereCenter.z < box->localMax.z);

						vec3 boxToSphereNormal;

						if (isInside) {
							// 깊은 침투(Deep Penetration) 처리: 구체가 박스 중심까지 파고들었을 때 가장 가까운 6개 면 중 하나를 찾아 밖으로 밀어냄
							float distToMinX = localSphereCenter.x - box->localMin.x;
							float distToMaxX = box->localMax.x - localSphereCenter.x;
							float distToMinY = localSphereCenter.y - box->localMin.y;
							float distToMaxY = box->localMax.y - localSphereCenter.y;
							float distToMinZ = localSphereCenter.z - box->localMin.z;
							float distToMaxZ = box->localMax.z - localSphereCenter.z;

							float minDist = distToMinX;
							vec3 localNormal = vec3(-1.0f, 0.0f, 0.0f);

							if (distToMaxX < minDist) { minDist = distToMaxX; localNormal = vec3(1.0f, 0.0f, 0.0f); }
							if (distToMinY < minDist) { minDist = distToMinY; localNormal = vec3(0.0f, -1.0f, 0.0f); }
							if (distToMaxY < minDist) { minDist = distToMaxY; localNormal = vec3(0.0f, 1.0f, 0.0f); }
							if (distToMinZ < minDist) { minDist = distToMinZ; localNormal = vec3(0.0f, 0.0f, -1.0f); }
							if (distToMaxZ < minDist) { minDist = distToMaxZ; localNormal = vec3(0.0f, 0.0f, 1.0f); }

							// 로컬 방향 벡터를 월드 방향 벡터로 변환 (w = 0.0f 주의)
							boxToSphereNormal = glm::normalize(vec3(boxTr->worldMatrix * vec4(localNormal, 0.0f)));
							penetration = sphere->worldRadius + minDist;
						}
						else {
							// 외부에서 표면만 부딪힌 일반적인 경우 (기존 로직 보강)
							float closestX = glm::clamp(localSphereCenter.x, box->localMin.x, box->localMax.x);
							float closestY = glm::clamp(localSphereCenter.y, box->localMin.y, box->localMax.y);
							float closestZ = glm::clamp(localSphereCenter.z, box->localMin.z, box->localMax.z);

							vec3 worldClosestPoint = vec3(boxTr->worldMatrix * vec4(closestX, closestY, closestZ, 1.0f));
							vec3 sphereDir = sphere->worldCenter - worldClosestPoint;
							float dist = glm::length(sphereDir);

							if (dist > 0.0001f) {
								boxToSphereNormal = sphereDir / dist;
								penetration = sphere->worldRadius - dist;
							}
							else {
								// 예외 방어
								boxToSphereNormal = vec3(0.0f, 1.0f, 0.0f);
								penetration = sphere->worldRadius;
							}
						}

						// A->B 방향에 맞게 Normal 설정
						normal = (colA->type == ColliderType::Box) ? boxToSphereNormal : -boxToSphereNormal;
					}

					//충돌 해결
					ResolveCollision(colA, colB, normal, penetration);
				}

				//A 입장에서의 B와 충돌 처리
				auto it = find(colA->collidingColliders.begin(), colA->collidingColliders.end(), colB);
				bool aAlreadyColliding = (it != colA->collidingColliders.end());
				if (isColliding) {
					if (!aAlreadyColliding) {
						//최초 충돌 발생
						colA->collidingColliders.push_back(colB);
						for (auto& comp : colA->gameObject->components) {
							comp->OnTriggerEnter(colB->gameObject);
						}
					}
					else {
						//계속 충돌 중
						for (auto& comp : colA->gameObject->components) {
							comp->OnTriggerStay(colB->gameObject);
						}
					}
				}
				else {
					if (aAlreadyColliding) {
						//충돌 종료
						colA->collidingColliders.erase(it);
						for (auto& comp : colA->gameObject->components) {
							comp->OnTriggerExit(colB->gameObject);
						}
					}
				}

				//B 입장에서의 A와 충돌 처리
				it = find(colB->collidingColliders.begin(), colB->collidingColliders.end(), colA);
				bool bAlreadyColliding = (it != colB->collidingColliders.end());
				if (isColliding) {
					if (!bAlreadyColliding) {
						//최초 충돌 발생
						colB->collidingColliders.push_back(colA);
						for (auto& comp : colB->gameObject->components) {
							comp->OnTriggerEnter(colA->gameObject);
						}
					}
					else {
						//계속 충돌 중
						for (auto& comp : colB->gameObject->components) {
							comp->OnTriggerStay(colA->gameObject);
						}
					}
				}
				else {
					if (bAlreadyColliding) {
						//충돌 종료
						colB->collidingColliders.erase(it);
						for (auto& comp : colB->gameObject->components) {
							comp->OnTriggerExit(colA->gameObject);
						}
					}
				}
			}
		}
	}
}

float ColliderManager::CombineFriction(float coefA, float coefB) {
	return (coefA + coefB) / 2.0f;
}

float ColliderManager::CombineRestitution(float restA, float restB) {
	return std::min(restA, restB);
}

void ColliderManager::ResolveCollisionWithFriction(Collider* colA, Collider* colB, vec3 collisionNormal) {
	RigidBody* rbA = colA->gameObject->GetComponent<RigidBody>();
	RigidBody* rbB = colB->gameObject->GetComponent<RigidBody>();

	if (!rbA && !rbB) return; // 두 오브젝트 모두 RigidBody가 없으면 충돌 해결 불가

	//최종 정적 / 동적 마찰 계수 계산
	float mu_s = CombineFriction(colA->staticFriction, colB->staticFriction);
	float mu_d = CombineFriction(colA->dynamicFriction, colB->dynamicFriction);

	// 상대 속도 계산
	vec3 velA = (rbA && !rbA->isKinematic) ? rbA->velocity : vec3(0.0f);
	vec3 velB = (rbB && !rbB->isKinematic) ? rbB->velocity : vec3(0.0f);
	vec3 relativeVel = velB - velA;

	//노말 방향 속도 성분
	float normalVelMag = glm::dot(relativeVel, collisionNormal);

	//두 무체가 서로 접근 중일 때만 마찰력 적용
	if (normalVelMag < 0.0f) {
		vec3 normalVelocity = normalVelMag * collisionNormal;
		vec3 tangentVelocity = relativeVel - normalVelocity;		//수평 접선 속도
		float tangentSpeed = glm::length(tangentVelocity);

		if (tangentSpeed > 0.0001f) {
			vec3 tangentDir = tangentVelocity / tangentSpeed; // 미끄러지는 방향
			float normalForceImpact = std::abs(normalVelMag); // 노말 방향 충격량

			vec3 frictionImpulse = vec3{ 0.0f };

			//정적 마찰력 판단(수평 속도가 작으면 정지)
			if (tangentSpeed <= normalForceImpact * mu_s) frictionImpulse = tangentVelocity;
			//동적 마찰력 적용(미끄러지는 동안 감속)
			else {
				float frictionDecel = normalForceImpact * mu_d;
				frictionImpulse = tangentDir * std::min(frictionDecel, tangentSpeed);
			}

			if (rbA && !rbA->isKinematic) rbA->velocity += frictionImpulse;
			if (rbB && !rbB->isKinematic) rbB->velocity -= frictionImpulse;
		}
	}
}

void ColliderManager::ResolveCollision(Collider* colA, Collider* colB, vec3 collisionNormal, float penetration) {
	RigidBody* rbA = colA->gameObject->GetComponent<RigidBody>();
	RigidBody* rbB = colB->gameObject->GetComponent<RigidBody>();
	if (!rbA && !rbB) return;

	float invMassA = (rbA && !rbA->isKinematic && rbA->mass > 0.0f) ? (1.0f / rbA->mass) : 0.0f;
	float invMassB = (rbB && !rbB->isKinematic && rbB->mass > 0.0f) ? (1.0f / rbB->mass) : 0.0f;
	if (invMassA + invMassB == 0.0f) return; // 두 오브젝트 모두 고정된 물체면 무시

	Transform* trA = colA->gameObject->GetComponent<Transform>();
	Transform* trB = colB->gameObject->GetComponent<Transform>();

	//위치 보정 (Penetration Correction)
	const float percent = 0.4f; // 보정 비율
	const float slop = 0.002f; // 허용 오차(지터링 방지)
	vec3 correction = std::max(penetration - slop, 0.0f) / (invMassA + invMassB) * percent * collisionNormal;

	if (invMassA > 0.0f) trA->Translate(-invMassA * correction, true);
	if (invMassB > 0.0f) trB->Translate(invMassB * correction, true);

	//탄성 충돌 속도 계산(Impulse Resolution)
	vec3 velA = (rbA && !rbA->isKinematic) ? rbA->velocity : vec3(0.0f);
	vec3 velB = (rbB && !rbB->isKinematic) ? rbB->velocity : vec3(0.0f);
	vec3 relativeVel = velB - velA;

	//충돌 수직 방향 상대 속도
	float velAlongNormal = glm::dot(relativeVel, collisionNormal);

	//물체가 이미 멀어지는 중이면 속도 변환 필요없음
	if (velAlongNormal > 0.0f) return;

	//반발 계수 계산
	float e = CombineRestitution(colA->bounciness, colB->bounciness);
	if (std::abs(velAlongNormal) < 0.2f) e = 0.0f; //충돌 속도가 낮으면 반발 계수 무시(충돌 후 튀지 않음)

	//충격량 크기 계산
	float j = -(1.0f + e) * velAlongNormal;
	j /= invMassA + invMassB;

	//충격량 벡터
	vec3 impulse = j * collisionNormal;

	//속도 적용
	if (invMassA > 0.0f) rbA->velocity -= invMassA * impulse;
	if (invMassB > 0.0f) rbB->velocity += invMassB * impulse;

	//마찰력 적용
	ResolveCollisionWithFriction(colA, colB, collisionNormal);
}