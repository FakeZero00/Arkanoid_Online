#include "SceneLoader.h"
#include <fstream>

void SceneLoader::LoadScene(AppContext& ctx, const string& sceneFile) {
	ifstream in{ sceneFile };
	if (not in) {
		cout << "json 파일을 읽지 못했습니다: " << sceneFile << endl;
		return;
	}

	json sceneData;
	in >> sceneData;

	map<string, Object*> spawnedObjects;

	//[Pass 1] 모든 오브젝트 생성
	for (const auto& objData : sceneData["Objects"]) {
		string name = objData["name"];
		Object* newObj = Instantiate(ctx, name);
		spawnedObjects[name] = newObj;

		//Transform 설정
		Transform* tr = newObj->GetComponent<Transform>();
		auto pos = objData["transform"]["position"];
		auto rot = objData["transform"]["rotation"];
		auto scale = objData["transform"]["scale"];

		tr->SetLocalPosition(pos[0], pos[1], pos[2]);
		tr->SetLocalRotation(rot[0], rot[1], rot[2]);
		tr->SetLocalScale(scale[0], scale[1], scale[2]);
	}

	//[Pass 2] 부모-자식 계층 구조(Hierarchy) 연결
	for (const auto& objData : sceneData["Objects"]) {
		string name = objData["name"];
		Object* obj = spawnedObjects[name];

		if (objData.contains("parent")) {
			string parentName = objData["parent"];
			if (!parentName.empty() && spawnedObjects.find(parentName) != spawnedObjects.end()) {
				Transform* parentTr = spawnedObjects[parentName]->GetComponent<Transform>();
				obj->GetComponent<Transform>()->SetParent(parentTr);
			}
		}
	}

	//[Pass 3] 최상위 루트(Root) 오브젝트들부터 시작해 전체 계층의 월드 행렬을 Top-Down으로 일괄 동기화
	for (const auto& objData : sceneData["Objects"]) {
		string name = objData["name"];
		Object* obj = spawnedObjects[name];
		Transform* tr = obj->GetComponent<Transform>();

		// 부모가 없는 최상위 조상인 경우에만 호출 (내부에서 자식들로 자동 재귀 전파됨)
		if (tr->parent == nullptr) {
			tr->CalculateWorldMatrix();
		}
	}

	//[Pass 4] 컴포넌트 부착
	for (const auto& objData : sceneData["Objects"]) {
		string name = objData["name"];
		Object* obj = spawnedObjects[name];

		if (objData.contains("components")) {
			for (const auto& compData : objData["components"]) {
				AttachComponent(obj, compData, spawnedObjects);
			}
		}
	}
}

void SceneLoader::AttachComponent(Object* obj, const json& compData, map<string, Object*> spawnedObjects) {
	string type = compData["type"];
	const json& props = compData.contains("properties") ? compData["properties"] : compData;

	// Registry에 해당 타입 이름이 등록되어 있는지 확인
	auto& registry = ComponentRegistry::GetRegistry();
	if (registry.find(type) != registry.end()) {
		registry[type](obj, props, spawnedObjects);
	}
	else {
		cout << "알 수 없는 컴포넌트 타입: " << type << std::endl;
	}
}

Object*	SceneLoader::Instantiate(AppContext& ctx, string objName) {
	auto newObj = make_unique<Object>(ctx, objName);
	Object* ptr = newObj.get();

	ctx.Hierarchy.push_back(move(newObj));
	return ptr;
}