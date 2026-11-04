#version 330 core
//버텍스 컬러
in vec3 out_Color;

//전달받은 색상
uniform vec4 tColor;

//출력할 색상 값
out vec4 FragColor;

void main()
{
	FragColor = tColor;
}