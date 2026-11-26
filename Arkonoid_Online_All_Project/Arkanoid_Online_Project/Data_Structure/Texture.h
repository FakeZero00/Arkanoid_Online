#pragma once
#include <GL/glew.h>
#include <string>

class Texture {
public:
	GLuint id = 0;
	int width = 0, height = 0, channels = 0;

	Texture(const std::string& filePath);
	~Texture();
	void Bind(GLuint unit = 0) const;
};