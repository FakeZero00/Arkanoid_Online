#version 330 core
//FragColor: 출력할 생상 값으로 프레임 버퍼로 전달됨

out vec4 FragColor;

void main()
{
	FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}