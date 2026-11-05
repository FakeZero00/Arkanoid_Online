#version 330 core
//FragColor: 출력할 생상 값으로 프레임 버퍼로 전달됨

//color: uniform 변수로 전달받은 색상 값
uniform vec4 color;

out vec4 FragColor;

void main()
{
	FragColor = color;
}