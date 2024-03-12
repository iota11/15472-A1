#version 450

//layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 1) uniform samplerCube cubeMapTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;


layout(location = 0) out vec4 outColor;

// ACES tone mapping function
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


const mat3 RGB_to_ACEScg3 = mat3(
    0.694245, 0.148186, 0.028873,
    0.228057, 0.737505, 0.034438,
    0.020713, 0.114209, 0.825214
);

vec3 LinearRGBToACEScg(vec3 linearRGB) {
    const mat3 RGB_to_ACEScg = mat3(
    1.0498110175, 0.0000000, -0.0000974845,
    -0.4959030231, 1.3733130458, 0.0982400361,
    0.00000, 0.00000, 0.9912520182
);

    return RGB_to_ACEScg * linearRGB;
}




vec3 LinearRGBToACEScg2(vec3 linearRGB) {
    const mat3 sRGB_to_ACEScg = mat3(
        1.705,  -0.623, -0.082,
       -0.130,   1.140, -0.010,
       -0.024,  -0.129,  1.287
    );
    return sRGB_to_ACEScg * linearRGB;
}


// Linear to sRGB conversion
vec3 LinearToSRGB(vec3 linearColor) {
    return mix(
        pow(linearColor, vec3(1.0 / 2.2)), // Apply gamma correction
        linearColor * 12.92, // Scale for low intensity
        lessThanEqual(linearColor, vec3(0.0031308)) // Conditional based on intensity
    );
}




void main() {
	//vec3 light = mix(vec3(0,0,0), vec3(1,1,1), dot(fragNormal, vec3(0,0,1)) * 0.5 + 0.5);
	vec3 col = texture(cubeMapTexture, fragNormal).xyz;
    col = ACESFilm(col);
    col = LinearToSRGB(col);
   //col = pow(col, vec3(1.0 / 2.2))
    outColor = vec4(col,1.0);
}