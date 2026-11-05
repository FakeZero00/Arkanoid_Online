#version 330 core
//Position: attribute index 0
//Color: attribute index 1

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vColor;

out vec3 out_Color;

//월드 변환 행렬
uniform mat4 model;

//뷰 변환 행렬
uniform mat4 view;

//투영 변환 행렬
uniform mat4 proj;

void main()
{	
	//정점 위치에 월드, 뷰, 투영 변환 행렬을 곱하여 최종 위치를 계산
	gl_Position = proj * view * model * vec4(vPos, 1.0f);
	out_Color = vColor;
}