#version 330 core
in vec3 out_Color;
in vec3 FragPos;
in vec3 Normal;

uniform vec4 tColor;		
uniform vec3 lightDir;		
uniform vec3 viewPos;		
uniform vec3 lightColor;	

out vec4 FragColor;

void main()
{
	// 1. Ambient (환경광)
	float ambientStrength = 0.2f;
	vec3 ambient = ambientStrength * lightColor;

	vec3 norm = length(Normal) > 0.0 ? normalize(Normal) : vec3(0.0, 1.0, 0.0);
	
	vec3 lightDirNormalized = normalize(lightDir);

	// 2. Diffuse (난반사광)
	float diff = max(dot(norm, lightDirNormalized), 0.0f);
    vec3 diffuse = diff * lightColor;

	// 3. Specular (정반사광)
	float shiness = 128.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirNormalized, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shiness);
    vec3 specular = spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * vec3(tColor);
	FragColor = vec4(result, tColor.a);
}