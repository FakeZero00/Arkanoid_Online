#include "MeshRenderer3D.h"
#include "Mesh.h"
#include "Material.h"
#include "Object.h"
#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include <gl/glew.h>
#include <gl/glm/gtc/type_ptr.hpp>
using namespace std;
using namespace glm;

extern map<string, Mesh*> meshChache;
extern map<string, Material*> materialChache;

MeshRenderer3D::MeshRenderer3D(Mesh* mesh, Material* material) : mesh(mesh) {
	this->materials.push_back(material);
}

void MeshRenderer3D::Render(Camera* activeCamera) {
	for (auto& material : materials) {
		//셰이더 사용
		material->Use();
		//VAO 바인딩
		mesh->Bind();

		//Transform의 worldMatrix를 셰이더에 전달
		Transform* transform = gameObject->GetComponent<Transform>();
		GLuint modelLoc = glGetUniformLocation(material->shaderProgram, "model");
		GLuint colorLoc = glGetUniformLocation(material->shaderProgram, "tColor");

		// View, Projection 행렬 전달
		if (activeCamera != nullptr) {
			GLuint viewLoc = glGetUniformLocation(material->shaderProgram, "view");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(activeCamera->GetViewMatrix()));

			GLuint projLoc = glGetUniformLocation(material->shaderProgram, "proj");
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(activeCamera->GetProjectionMatrix()));

			//Phong 모델을 위한 카메라의 월드 위치(viewPos) 전달
			Transform* camTr = activeCamera->gameObject->GetComponent<Transform>();
			GLuint viewPosLoc = glGetUniformLocation(material->shaderProgram, "viewPos");
			glUniform3fv(viewPosLoc, 1, value_ptr(camTr->worldPosition));
		}

		//씬(Hierarchy)에서 Light 컴포넌트 탐색 및 조명 정보 전달
		Light* sceneLight = nullptr;
		Transform* lightTr = nullptr;
		for (const auto& obj : gameObject->ctx.Hierarchy) {
			if (!obj->isValid) continue;
			sceneLight = obj->GetComponent<Light>();
			if (sceneLight != nullptr) {
				lightTr = obj->GetComponent<Transform>();
				break; // 첫 번째 조명을 찾으면 루프 종료
			}
		}

		// Uniform 위치 가져오기 및 값 전달 로직
		GLuint lightDirLoc = glGetUniformLocation(material->shaderProgram, "lightDir");
		GLuint lightColorLoc = glGetUniformLocation(material->shaderProgram, "lightColor");

		if (sceneLight != nullptr && lightTr != nullptr) {
			// 💡 Unity 스타일 Directional Light: position 무시, rotation만 반영
			// 라이트 오브젝트의 로컬 전방 벡터(0, 0, -1)에 월드 회전 행렬을 곱하여 방향 추출 (w=0으로 평행이동 차단)
			vec3 lightDir = normalize(vec3(lightTr->worldMatrix * vec4(0.0f, 0.0f, -1.0f, 0.0f)));
			vec3 finalLightColor = vec3(sceneLight->lightColor) * sceneLight->intensity;

			glUniform3fv(lightDirLoc, 1, value_ptr(lightDir));
			glUniform3fv(lightColorLoc, 1, value_ptr(finalLightColor));
		}
		else {
			glUniform3f(lightDirLoc, 0.0f, 0.0f, -1.0f);
			glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
		}

		//Pass1 : 외곽선 렌더링 처리
		if (material->isOutline) {
			mat4 outlineMatrix = transform->worldMatrix;
			outlineMatrix = glm::scale(outlineMatrix, vec3{ 1.5f }); //외곽선 확대
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(outlineMatrix));
			glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f); //외곽선 색상 검정
			glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_BYTE, 0);

			//원래 색상 전달
			vec4 originalColor = material->GetVec4("tColor");
			glUniform4f(colorLoc, originalColor.r, originalColor.g, originalColor.b, originalColor.a);
		}

		//Pass2 : Mesh 데이터 그리기
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(transform->worldMatrix));
		if (isValid) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
		}
		if (isWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		glBindVertexArray(0);
	}
}

Component* MeshRenderer3D::CreateFromJson(Object* obj, const json& props, const map<string, Object*>& spawnedObjects) {
	if (props.contains("mesh") == false || props.contains("materials") == false) {
		cout << obj->name << " 오브젝트의 MeshRenderer3D 컴포넌트에 mesh와 materials 속성이 필요합니다." << endl;
		return nullptr;
	}

	auto matData = props["materials"][0];
	Material* material = materialChache[matData];

	if (matData.contains("boolUniforms")) {
		for (auto& [key, val] : matData["boolUniforms"].items()) {
			material->SetBool(key, val);
		}
	}
	if (matData.contains("intUniforms")) {
		for (auto& [key, val] : matData["intUniforms"].items()) {
			material->SetInt(key, val);
		}
	}
	if (matData.contains("floatUniforms")) {
		for (auto& [key, val] : matData["floatUniforms"].items()) {
			material->SetFloat(key, val);
		}
	}
	if (matData.contains("vec2Uniforms")) {
		for (auto& [key, val] : matData["vec2Uniforms"].items()) {
			material->SetVec2(key, vec2(val[0], val[1]));
		}
	}
	if (matData.contains("vec3Uniforms")) {
		for (auto& [key, val] : matData["vec3Uniforms"].items()) {
			material->SetVec3(key, vec3(val[0], val[1], val[2]));
		}
	}
	if (matData.contains("vec4Uniforms")) {
		for (auto& [key, val] : matData["vec4Uniforms"].items()) {
			material->SetVec4(key, vec4(val[0], val[1], val[2], val[3]));
		}
	}

	MeshRenderer3D* meshRenderer = obj->AddComponent<MeshRenderer3D>(meshChache[props["mesh"]], material);

	if (props.contains("isWireframe")) meshRenderer->isWireframe = props["isWireframe"];

	return meshRenderer;
}

REGISTER_COMPONENT(MeshRenderer3D)