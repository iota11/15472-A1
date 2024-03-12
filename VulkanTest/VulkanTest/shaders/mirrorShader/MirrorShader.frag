#version 450

//layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform samplerCube cubeMapTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 camPos;
layout(location = 4) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 viewDir = normalize(fragPosition - camPos);
	vec3 normal = normalize(fragNormal);
	vec3 reflection = reflect(viewDir, normal);
	outColor = texture(cubeMapTexture, reflection);
}