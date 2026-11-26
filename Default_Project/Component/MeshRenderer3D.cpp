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

MeshRenderer3D::MeshRenderer3D(Mesh* mesh, const vector<Material*>& materials)
	: mesh(mesh), materials(materials) {}

void MeshRenderer3D::Render(Camera* activeCamera) {
	if (mesh == nullptr || materials.empty()) return;

	Transform* transform = gameObject->GetComponent<Transform>();
	const auto& lights = gameObject->ctx.lights;
	int numLights = std::min(static_cast<int>(lights.size()), 10);

	// 서브메쉬 0번부터 차례대로 순회하며 i번 마테리얼 적용
	for (size_t i = 0; i < mesh->subMeshes.size(); ++i) {
		const SubMesh& subMesh = mesh->subMeshes[i];
		if (subMesh.indexCount <= 0) continue;

		// i번 마테리얼 선택 (JSON에 등록된 마테리얼 수가 부족하면 마지막 마테리얼 사용)
		Material* material = (i < materials.size()) ? materials[i] : materials.back();
		if (material == nullptr) continue;

		// 셰이더 및 VAO, 해당 서브메쉬의 EBO 바인딩
		material->Use();
		mesh->Bind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.EBO);

		GLuint modelLoc = glGetUniformLocation(material->shaderProgram, "model");
		GLuint colorLoc = glGetUniformLocation(material->shaderProgram, "tColor");

		// View, Projection 행렬 전달
		if (activeCamera != nullptr) {
			GLuint viewLoc = glGetUniformLocation(material->shaderProgram, "view");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(activeCamera->GetViewMatrix()));

			GLuint projLoc = glGetUniformLocation(material->shaderProgram, "proj");
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(activeCamera->GetProjectionMatrix()));

			Transform* camTr = activeCamera->gameObject->GetComponent<Transform>();
			GLuint viewPosLoc = glGetUniformLocation(material->shaderProgram, "viewPos");
			glUniform3fv(viewPosLoc, 1, value_ptr(camTr->worldPosition));
		}

		// 1. 씬에 등록된 라이트 개수 전달
		glUniform1i(glGetUniformLocation(material->shaderProgram, "numLights"), numLights);

		// 2. 광원 정보 전달
		for (int l = 0; l < numLights; ++l) {
			Light* sceneLight = lights[l];
			Transform* lightTr = sceneLight->gameObject->GetComponent<Transform>();
			if (lightTr == nullptr) continue;

			string baseName = "lights[" + to_string(l) + "].";

			int typeInt = static_cast<int>(sceneLight->lightType);
			vec3 finalLightColor = vec3(sceneLight->lightColor) * sceneLight->intensity;
			vec3 lightDir = normalize(vec3(lightTr->worldMatrix * vec4(0.0f, 0.0f, -1.0f, 0.0f)));

			glUniform1i(glGetUniformLocation(material->shaderProgram, (baseName + "type").c_str()), typeInt);
			glUniform3fv(glGetUniformLocation(material->shaderProgram, (baseName + "color").c_str()), 1, value_ptr(finalLightColor));
			glUniform3fv(glGetUniformLocation(material->shaderProgram, (baseName + "direction").c_str()), 1, value_ptr(lightDir));

			if (sceneLight->lightType == LightType::Spot || sceneLight->lightType == LightType::Point) {
				vec3 lightPos = vec3(lightTr->worldMatrix[3]);
				glUniform3fv(glGetUniformLocation(material->shaderProgram, (baseName + "position").c_str()), 1, value_ptr(lightPos));
				glUniform1f(glGetUniformLocation(material->shaderProgram, (baseName + "radius").c_str()), sceneLight->radius);

				if (sceneLight->lightType == LightType::Spot) {
					glUniform1f(glGetUniformLocation(material->shaderProgram, (baseName + "cutOff").c_str()), glm::cos(glm::radians(sceneLight->cutOff)));
					glUniform1f(glGetUniformLocation(material->shaderProgram, (baseName + "outerCutOff").c_str()), glm::cos(glm::radians(sceneLight->outerCutOff)));
				}
			}
		}

		// Pass1 : 외곽선 렌더링 처리
		if (material->isOutline) {
			mat4 outlineMatrix = transform->worldMatrix;
			outlineMatrix = glm::scale(outlineMatrix, vec3{ 1.5f });
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(outlineMatrix));
			glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
			glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, 0);

			vec4 originalColor = material->GetVec4("tColor");
			glUniform4f(colorLoc, originalColor.r, originalColor.g, originalColor.b, originalColor.a);
		}

		// Pass2 : 서브메쉬 데이터 그리기
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(transform->worldMatrix));
		if (isValid) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, 0);
		}
		if (isWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, 0);
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

	vector<Material*> loadedMaterials;

	// materials 배열의 0번 요소부터 순서대로 로드
	for (const auto& matData : props["materials"]) {
		string matKey = matData.is_string() ? matData.get<string>() : matData.value("name", "");
		if (matKey.empty() || materialChache.find(matKey) == materialChache.end()) {
			cout << obj->name << " 오브젝트의 Material을 찾을 수 없습니다: " << matKey << endl;
			continue;
		}

		Material* material = materialChache[matKey];

		// matData가 객체 형태로 유니폼 오버라이드를 포함한 경우 처리
		if (matData.is_object()) {
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
		}

		loadedMaterials.push_back(material);
	}

	MeshRenderer3D* meshRenderer = obj->AddComponent<MeshRenderer3D>(meshChache[props["mesh"]], loadedMaterials);

	if (props.contains("isWireframe")) meshRenderer->isWireframe = props["isWireframe"];

	return meshRenderer;
}

REGISTER_COMPONENT(MeshRenderer3D)