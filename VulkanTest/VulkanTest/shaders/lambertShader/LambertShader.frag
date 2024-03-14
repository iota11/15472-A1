#version 450

//layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform samplerCube cubeMapTexture;
layout(binding = 2) uniform sampler2D albedoTexture;
layout(binding = 3) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D displacementTexture;
layout(binding = 3) uniform sampler2D roughnessTexture;
layout(binding = 2) uniform sampler2D mentallicTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 camPos;
layout(location = 4) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;


// ACES tone mapping function from https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 LinearToSRGB(vec3 linearColor) {
    return mix(
        pow(linearColor, vec3(1.0 / 2.2)), // Apply gamma correction
        linearColor * 12.92, // Scale for low intensity
        lessThanEqual(linearColor, vec3(0.0031308)) // Conditional based on intensity
    );
}

void main() {
	vec3 viewDir = normalize(fragPosition - camPos);
	vec3 col = texture(albedoTexture, fragTexCoord).xyz;

	//col = ACESFilm(col);
    col = LinearToSRGB(col);
   //col = pow(col, vec3(1.0 / 2.2))
    outColor = vec4(col,1.0);
}