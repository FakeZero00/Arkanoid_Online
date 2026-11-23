#version 330 core
//Position: attribute index 0
//Color: attribute index 1

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec3 vNormal;

out vec3 out_Color;
out vec3 FragPos;
out vec3 Normal;

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

	//월드 좌표계에서의 정점 위치를 계산
	FragPos = vec3(model * vec4(vPos, 1.0f));

	//부등스케일(non-uniform scale) 시 노말 찌그러짐 방지를 위해 역전치 행렬 사용
	Normal = mat3(transpose(inverse(model))) * vNormal;
}