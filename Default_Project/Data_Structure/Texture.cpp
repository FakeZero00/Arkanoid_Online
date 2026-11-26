#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include <iostream>

Texture::Texture(const std::string& filePath) {
	stbi_set_flip_vertically_on_load(true); // OpenGL UV 좌표계(좌하단 원점)에 맞춤
	unsigned char* data = stbi_load(("Assets/" + filePath).c_str(), &width, &height, &channels, 0);

	if (data) {
		GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data);
	}
	else {
		std::cout << "텍스처 로드 실패: " << filePath << std::endl;
	}
}

Texture::~Texture() {
	if (id != 0) glDeleteTextures(1, &id);
}

void Texture::Bind(GLuint unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, id);
}