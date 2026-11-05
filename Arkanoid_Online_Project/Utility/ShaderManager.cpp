#include "ShaderManager.h"
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

string filetobuf(string path)
{
	ifstream in("Shader/" + path);
	if (not in) {
		cerr << "파일 열기 실패: " << path << endl;
		exit(1);
	}
	stringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

//버텍스 셰이더 생성
void make_vertexShaders(map<string, GLuint>& shaders, string path)
{
	//셰이더 코드 읽어서 string -> char* 변환
	string verexSoure = filetobuf(path);
	const char* source = verexSoure.c_str();

	//셰이더 생성
	shaders[path] = glCreateShader(GL_VERTEX_SHADER);

	//셰이더 소스 코드 설정 및 컴파일
	glShaderSource(shaders[path], 1, &source, NULL);
	glCompileShader(shaders[path]);

	//에러 체크
	GLint result;
	GLchar errorLog[512];

	glGetShaderiv(shaders[path], GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderInfoLog(shaders[path], 512, NULL, errorLog);
		cerr << "버텍스 셰이더 컴파일 실패[" << path << "]: " << errorLog << endl;
		exit(1);
	}
}

//프래그먼트 셰이더 생성
void make_fragmentShaders(map<string, GLuint>& shaders, string path)
{
	//셰이더 코드 읽어서 string -> char* 변환
	string fragmentSource = filetobuf(path);
	const char* source = fragmentSource.c_str();

	//셰이더 생성
	shaders[path] = glCreateShader(GL_FRAGMENT_SHADER);

	//셰이더 소스 코드 설정 및 컴파일
	glShaderSource(shaders[path], 1, &source, NULL);
	glCompileShader(shaders[path]);

	//에러 체크
	GLint result;
	GLchar errorLog[512];

	glGetShaderiv(shaders[path], GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderInfoLog(shaders[path], 512, NULL, errorLog);
		cerr << "프래그먼트 셰이더 컴파일 실패[" << path << "]: " << errorLog << endl;
		exit(1);
	}
}

void make_shaderProgram(string name, map<string, GLuint>& shaders, GLuint vertexShader, GLuint fragmentShader)
{
	//셰이더 프로그램 생성
	GLuint shaderProgram = glCreateProgram();

	//셰이더 프로그램에 셰이더 연결
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	//셰이더 프로그램 링크
	glLinkProgram(shaderProgram);

	//링크 후 객체 삭제
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//에러 체크
	GLint result;
	GLchar errorLog[512];

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, errorLog);
		cerr << "셰이더 프로그램 링크 실패[" << name << "]: " << errorLog << endl;
		exit(1);
	}

	shaders[name] = shaderProgram;
}

void initBuffer(GLuint& VAO, GLuint& VBO, GLuint& EBO, const float* data, GLsizeiptr dataSize, const GLubyte* indices, GLsizeiptr indexSize)
{
	//VAO 객체 생성 및 바인딩
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//VBO 객체 생성 및 바인딩
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//EBO 객체 생성 및 바인딩 및 데이터 설정
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, indices, GL_STATIC_DRAW);

	//버텍스 데이터 설정
	glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);

	//버텍스 좌표: 속성 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//버텍스 색상: 속성 1
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}