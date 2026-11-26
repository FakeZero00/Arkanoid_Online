#version 330 core
in vec3 out_Color;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec4 tColor;				
uniform vec3 viewPos;

uniform sampler2D mainTex;
uniform bool useTexture;

//다중 광원을 받기 위한 구조체 정의
struct Light {
    int type; // 0: Directional, 1: Point, 2: Spot
    vec3 position;
    vec3 direction;
    vec3 color;

    float radius;
    
    float cutOff;
    float outerCutOff;
};

#define MAX_LIGHTS 10
uniform Light lights[MAX_LIGHTS]; // 빛 정보 배열
uniform int numLights;            // 현재 씬에 존재하는 활성화된 빛의 개수

out vec4 FragColor;

void main()
{
	vec4 baseColor = tColor;
	if (useTexture) {
		baseColor *= texture(mainTex, TexCoord);
	}

	vec3 norm = length(Normal) > 0.0 ? normalize(Normal) : vec3(0.0, 1.0, 0.0);
	vec3 viewDir = normalize(viewPos - FragPos);
	
	vec3 finalAmbientDiffuse = vec3(0.0); // 누적할 최종 조명 색상
	vec3 finalSpecular = vec3(0.0); // 누적할 최종 정반사광 색상

	// 씬에 존재하는 모든 빛을 순회하며 조명 값 합산
	int count = min(numLights, MAX_LIGHTS);
	for(int i = 0; i < count; i++) {
		
		// 1. Ambient (환경광)
		float ambientStrength = 0.2f;
		vec3 ambient = ambientStrength * lights[i].color;

		vec3 lightDirNormalized;
		float attenuation = 1.0;
		float spotIntensity = 1.0;

		if (lights[i].type == 0) {
			// Directional Light
			lightDirNormalized = normalize(-lights[i].direction);
		} else {
			vec3 lightToFrag = FragPos - lights[i].position;
			float distance = length(lightToFrag);
			lightDirNormalized = normalize(lightToFrag);

			// 거리 감쇠 (Attenuation)
			attenuation = 0.0;
			if (distance < lights[i].radius) {
				// 거리 비율 (0.0 ~ 1.0)
				float d = distance / lights[i].radius;
				
				// 반경 끝에서 빛이 부드럽게 0이 되도록 깎아주는 윈도우 함수
				float falloff = clamp(1.0 - (d * d * d * d), 0.0, 1.0);
				
				// 최종 감쇠 = 역제곱 법칙 * 윈도우 함수
				attenuation = (falloff * falloff) / (distance * distance + 1.0);
			}

			// 스팟 원뿔 각도 (Spotlight CutOff)
			if (lights[i].type == 2) {
				// 💡 핵심: 스팟 라이트일 때만 direction 벡터를 반대로(-lights[i].direction) 뒤집어 내적합니다.
				float theta = dot(lightDirNormalized, normalize(-lights[i].direction));
				float epsilon = lights[i].cutOff - lights[i].outerCutOff;
				spotIntensity = clamp((theta - lights[i].outerCutOff) / epsilon, 0.0, 1.0);
			}
		}

		// 2. Diffuse (난반사광)
		float diff = max(dot(norm, -lightDirNormalized), 0.0f);
		vec3 diffuse = diff * lights[i].color;

		// 3. Specular (정반사광)
		float shiness = 32.0;
		vec3 reflectDir = reflect(lightDirNormalized, norm);  
		float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shiness);
		vec3 specular = spec * lights[i].color;

		diffuse *= attenuation * spotIntensity;
		specular *= attenuation * spotIntensity;

		// 누적
		finalAmbientDiffuse += (ambient + diffuse);
		finalSpecular += specular;
	}

	vec3 result = (finalAmbientDiffuse * vec3(baseColor)) + (finalAmbientDiffuse * 0.1) + finalSpecular;
	FragColor = vec4(result, baseColor.a);
}