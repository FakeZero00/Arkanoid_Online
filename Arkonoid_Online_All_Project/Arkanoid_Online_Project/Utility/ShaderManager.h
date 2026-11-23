#pragma once
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <string>
#include <map>
using namespace std;

void make_vertexShaders(map<string, GLuint>& shaders, string path);
void make_fragmentShaders(map<string, GLuint>& shaders, string path);
void make_shaderProgram(string name, map<string, GLuint>& shaders, GLuint vertexShader, GLuint fragmentShader);
void initBuffer(GLuint& VAO, GLuint& VBO, GLuint& EBO, const float* data, GLsizeiptr dataSize, const GLubyte* indices, GLsizeiptr indexSize);