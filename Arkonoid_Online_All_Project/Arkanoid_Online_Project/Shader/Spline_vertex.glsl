#version 330 core
//Position: attribute index 0

layout(location = 0) in vec3 vPos;

//월드 변환 행렬
uniform mat4 model;

//뷰 변환 행렬
uniform mat4 view;

//투영 변환 행렬
uniform mat4 proj;

void main()
{	
	//정점 위치에 월드 변환 행렬을 곱하여 최종 위치를 계산
	gl_Position = proj * view * model * vec4(vPos, 1.0f);
}