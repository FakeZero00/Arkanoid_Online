#include "Material.h"

void Material::Use() {
	glUseProgram(shaderProgram);

	//저장된 bool 값들을 셰이더의 uniform으로 일괄 전송
	for (const auto& pair : boolUniforms) {
		GLint boolLoc = glGetUniformLocation(shaderProgram, pair.first.c_str());
		if (boolLoc == -1) continue;
		glUniform1i(boolLoc, pair.second);
	}

	//저장된 int 값들을 셰이더의 uniform으로 일괄 전송
	for (const auto& pair : intUniforms) {
		GLint intLoc = glGetUniformLocation(shaderProgram, pair.first.c_str());
		if (intLoc == -1) continue;
		glUniform1i(intLoc, pair.second);
	}

	//저장된 float 값들을 셰이더의 uniform으로 일괄 전송
	for (const auto& pair : floatUniforms) {
		GLint floatLoc = glGetUniformLocation(shaderProgram, pair.first.c_str());
		if (floatLoc == -1) continue;
		glUniform1f(floatLoc, pair.second);
	}

	//저장된 vec2 값들을 셰이더의 uniform으로 일괄 전송
	for (const auto& pair : vec2Uniforms) {
		GLint vec2Loc = glGetUniformLocation(shaderProgram, pair.first.c_str());
		if (vec2Loc == -1) continue;
		glUniform2f(vec2Loc, pair.second.x, pair.second.y);
	}

	//저장된 vec3 값들을 셰이더의 uniform으로 일괄 전송
	for (const auto& pair : vec3Uniforms) {
		GLint vec3Loc = glGetUniformLocation(shaderProgram, pair.first.c_str());
		if (vec3Loc == -1) continue;
		glUniform3f(vec3Loc, pair.second.x, pair.second.y, pair.second.z);
	}

	//저장된 vec4 값들을 셰이더의 uniform으로 일괄 전송
	for (const auto& pair : vec4Uniforms) {
		GLint vec4Loc = glGetUniformLocation(shaderProgram, pair.first.c_str());
		if (vec4Loc == -1) continue;
		glUniform4f(vec4Loc, pair.second.x, pair.second.y, pair.second.z, pair.second.w);
	}
}

//세터 함수
void Material::SetBool(const string& name, bool value) {
	boolUniforms[name] = value;
}

void Material::SetInt(const string& name, int value) {
	intUniforms[name] = value;
}

void Material::SetFloat(const string& name, float value) {
	floatUniforms[name] = value;
}

void Material::SetVec2(const string& name, const vec2& value) {
	vec2Uniforms[name] = value;
}

void Material::SetVec3(const string& name, const vec3& value) {
	vec3Uniforms[name] = value;
}

void Material::SetVec4(const string& name, const vec4& value) {
	vec4Uniforms[name] = value;
}

//게터 함수
bool Material::GetBool(const string& name) {
	return boolUniforms[name];
}

int Material::GetInt(const string& name) {
	return intUniforms[name];
}

float Material::GetFloat(const string& name) {
	return floatUniforms[name];
}

vec2 Material::GetVec2(const string& name) {
	return vec2Uniforms[name];
}

vec3 Material::GetVec3(const string& name) {
	return vec3Uniforms[name];
}

vec4 Material::GetVec4(const string& name) {
	return vec4Uniforms[name];
}