#pragma once
#include <map>
#include <string>
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
using namespace std;
using namespace glm;

class Material {
public:
	GLuint shaderProgram;

	//타입별 Uniform 변수 저장 map
	map<string, bool> boolUniforms;
	map<string, int> intUniforms;
	map<string, float> floatUniforms;
	map<string, vec2> vec2Uniforms;
	map<string, vec3> vec3Uniforms;
	map<string, vec4> vec4Uniforms;

	//Pass 제어 변수
	bool isOutline = false;

	Material(GLuint programID) : shaderProgram(programID) {}

	void Use();

	void SetBool(const string& name, bool value);
	void SetInt(const string& name, int value);
	void SetFloat(const string& name, float value);
	void SetVec2(const string& name, const vec2& value);
	void SetVec3(const string& name, const vec3& value);
	void SetVec4(const string& name, const vec4& value);

	bool GetBool(const string& name);
	int GetInt(const string& name);
	float GetFloat(const string& name);
	vec2 GetVec2(const string& name);
	vec3 GetVec3(const string& name);
	vec4 GetVec4(const string& name);
};