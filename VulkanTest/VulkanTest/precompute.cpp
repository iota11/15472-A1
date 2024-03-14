
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <random>
#include <sstream>
#include <algorithm> 
#include <functional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stb_image.h>

#include "precompute.hpp"

//this file is to precompute the irrediance LUT, BRDF LUT.



enum Face {
	PositiveX = 0, NegativeX = 1,
	PositiveY = 2, NegativeY = 3,
	PositiveZ = 4, NegativeZ = 5,
};


// Convert RGBE to RGBA (float)
void PreComputeManager::rgbeToRgbaFloat(uint8_t* rgbeData, float* rgbaData, size_t numPixels) {
	for (size_t i = 0; i < numPixels; ++i) {
		// Extract RGBE components
		uint8_t r = rgbeData[i * 4];
		uint8_t g = rgbeData[i * 4 + 1];
		uint8_t b = rgbeData[i * 4 + 2];
		uint8_t e = rgbeData[i * 4 + 3];

		// Calculate RGB components
		float factor = std::ldexp(1.0f, e - 128); // Compute 2^(E-128)
		float rf = (static_cast<float>(r) + 0.5f) * factor / 256.0f;
		float gf = (static_cast<float>(g) + 0.5f) * factor / 256.0f;
		float bf = (static_cast<float>(b) + 0.5f) * factor / 256.0f;

		// Set alpha component
		float a = 1.0f; // Fixed value for alpha (fully opaque)

		// Store RGBA components
		rgbaData[i * 4] = rf;
		rgbaData[i * 4 + 1] = gf;
		rgbaData[i * 4 + 2] = bf;
		rgbaData[i * 4 + 3] = a;
	}
}



glm::vec4 float_to_rgbe( const glm::vec3& rgb) {
	unsigned char out[4];
	float maxVal = std::max({ rgb.x, rgb.y, rgb.z });
	if (maxVal < 1e-32) {
		out[0] = out[1] = out[2] = out[3] = 0;
	}
	else {
		int e;
		// Normalize so maxVal is 1.0
		float normalized = std::frexp(maxVal, &e);
		normalized = normalized * 256.0f / maxVal;
		out[0] = static_cast<unsigned char>(rgb.x * normalized);
		out[1] = static_cast<unsigned char>(rgb.y * normalized);
		out[2] = static_cast<unsigned char>(rgb.z * normalized);
		out[3] = static_cast<unsigned char>(e + 128); // Store exponent
	}
	glm::vec4 value = glm::vec4(out[0], out[1], out[2], out[3]);
	return value;
}

//reference:https://github.com/ixchow/15-466-ibl/tree/master/cube/blur_cube.cpp
void PreComputeManager::DiffuseCompute(const std::string &filepath, const std::string &outpath) {
	int32_t samples = 1000;
	std::function< glm::vec3() > make_sample;
	std::function< glm::vec3(glm::vec3) > sum_bright_directions; //run lighting for bright directions, given normal
	make_sample = []() -> glm::vec3 {
		//attempt to importance sample upper hemisphere (cos-weighted):
		//based on: http://www.rorydriscoll.com/2009/01/07/better-sampling/
		static std::mt19937 mt(0x12341234);
		glm::vec2 rv(mt() / float(mt.max()), mt() / float(mt.max()));
		float phi = rv.x * 2.0f * M_PI;
		float r = std::sqrt(rv.y);
		return glm::vec3(
			std::cos(phi) * r,
			std::sin(phi) * r,
			std::sqrt(1.0f - rv.y)
		);
	};
	struct BrightDirection {
		glm::vec3 direction = glm::vec3(0.0f);
		glm::vec3 light = glm::vec3(0.0f); //already multiplied by solid angle, I guess?
	};
	std::vector< BrightDirection > bright_directions;
	sum_bright_directions = [&bright_directions](glm::vec3 n) -> glm::vec3 {
		glm::vec3 ret = glm::vec3(0.0f);
		for (auto const& bd : bright_directions) {
			ret += std::max(0.0f, glm::dot(bd.direction, n)) * bd.light;
		}
		return ret;
	};
	/// <summary>
	/// feed data
	/// </summary>

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	int pixelcount = texWidth * texHeight;
	float* rgba_pixels = (float*)malloc(pixelcount * 4 * sizeof(float));
	rgbeToRgbaFloat(pixels, rgba_pixels, pixelcount);
	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	stbi_image_free(pixels);
	std::cout << "Converting to linear floating point..."; std::cout.flush();
	std::vector< glm::vec3 > in_data;
	for (int i = 0; i < pixelcount * 4; i += 4) {
		// Convert each RGBA pixel to an RGB glm::vec3 and add it to in_data
		glm::vec3 rgbPixel(rgba_pixels[i], rgba_pixels[i + 1], rgba_pixels[i + 2]);
		in_data.push_back(rgbPixel);
	}
	glm::uvec2 in_size;
	glm::uvec2 out_size;
	in_size.x = texWidth;
	in_size.y = texHeight;
	out_size.x = 128;
	out_size.y = 128 * 6;
	free(rgba_pixels);
	std::cout << " done." << std::endl;



	if (sum_bright_directions) {
		uint32_t bright = std::min< uint32_t >(in_data.size(), brightest);
		std::cout << "Separating the brightest " << bright << " pixels..."; std::cout.flush();
		std::vector< std::pair< float, uint32_t > > pixels;
		pixels.reserve(in_data.size());
		for (auto const& px : in_data) {
			pixels.emplace_back(std::max(px.r, std::max(px.g, px.b)), pixels.size());
		}
		std::stable_sort(pixels.begin(), pixels.end());
		for (uint32_t b = 0; b < bright; ++b) {
			uint32_t i = pixels[pixels.size() - 1 - b].second;
			uint32_t s = i % in_size.x;
			uint32_t t = (i / in_size.x) % in_size.x;
			uint32_t f = i / (in_size.x * in_size.x);

			glm::vec3 sc; //maps to rightward axis on face
			glm::vec3 tc; //maps to upward axis on face
			glm::vec3 ma; //direction to face
			//See OpenGL 4.4 Core Profile specification, Table 8.18:
			if (f == PositiveX) { sc = glm::vec3(0.0f, 0.0f, -1.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(1.0f, 0.0f, 0.0f); }
			else if (f == NegativeX) { sc = glm::vec3(0.0f, 0.0f, 1.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(-1.0f, 0.0f, 0.0f); }
			else if (f == PositiveY) { sc = glm::vec3(1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, 0.0f, 1.0f); ma = glm::vec3(0.0f, 1.0f, 0.0f); }
			else if (f == NegativeY) { sc = glm::vec3(1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, 0.0f, -1.0f); ma = glm::vec3(0.0f, -1.0f, 0.0f); }
			else if (f == PositiveZ) { sc = glm::vec3(1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(0.0f, 0.0f, 1.0f); }
			else if (f == NegativeZ) { sc = glm::vec3(-1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(0.0f, 0.0f, -1.0f); }
			else assert(0 && "Invalid face.");

			glm::vec3 test = ma + (2.0f * (s + 0.5f) / in_size.x - 1.0f) * sc + (2.0f * (t + 0.5f) / in_size.x - 1.0f) * tc;
			glm::vec3 dir = glm::normalize(test);

			bright_directions.emplace_back();
			bright_directions.back().direction = dir;
			float solid_angle = 4.0f * M_PI / float(6.0f * in_size.x * in_size.x); // approximate, since pixels on cube actually take up different amounts depending on position
			bright_directions.back().light = in_data[i] * solid_angle;

			in_data[i] = glm::vec3(0.0f, 0.0f, 0.0f); //remove from input data
		}
		std::cout << " done." << std::endl;
	}

	auto lookup = [&in_data, &in_size](glm::vec3 const& dir) -> glm::vec3 {
		float sc, tc, ma;
		uint32_t f;
		if (std::abs(dir.x) >= std::abs(dir.y) && std::abs(dir.x) >= std::abs(dir.z)) {
			if (dir.x >= 0) { sc = -dir.z; tc = -dir.y; ma = dir.x; f = PositiveX; }
			else { sc = dir.z; tc = -dir.y; ma = -dir.x; f = NegativeX; }
		}
		else if (std::abs(dir.y) >= std::abs(dir.z)) {
			if (dir.y >= 0) { sc = dir.x; tc = dir.z; ma = dir.y; f = PositiveY; }
			else { sc = dir.x; tc = -dir.z; ma = -dir.y; f = NegativeY; }
		}
		else {
			if (dir.z >= 0) { sc = dir.x; tc = -dir.y; ma = dir.z; f = PositiveZ; }
			else { sc = -dir.x; tc = -dir.y; ma = -dir.z; f = NegativeZ; }
		}

		int32_t s = std::floor(0.5f * (sc / ma + 1.0f) * in_size.x);
		s = std::max(0, std::min(int32_t(in_size.x) - 1, s));
		int32_t t = std::floor(0.5f * (tc / ma + 1.0f) * in_size.x);
		t = std::max(0, std::min(int32_t(in_size.x) - 1, t));

		return in_data[(f * in_size.x + t) * in_size.x + s];
	};

	std::vector< glm::vec3 > out_data;
	out_data.reserve(out_size.x * out_size.y);

	std::cout << "Using " << samples << " samples per texel." << std::endl;

	for (uint32_t f = 0; f < 6; ++f) {
		std::cout << "Sampling face " << f << "/6 ..."; std::cout.flush();
		glm::vec3 sc; //maps to rightward axis on face
		glm::vec3 tc; //maps to upward axis on face
		glm::vec3 ma; //direction to face
		//See OpenGL 4.4 Core Profile specification, Table 8.18:
		if (f == PositiveX) { sc = glm::vec3(0.0f, 0.0f, -1.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(1.0f, 0.0f, 0.0f); }
		else if (f == NegativeX) { sc = glm::vec3(0.0f, 0.0f, 1.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(-1.0f, 0.0f, 0.0f); }
		else if (f == PositiveY) { sc = glm::vec3(1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, 0.0f, 1.0f); ma = glm::vec3(0.0f, 1.0f, 0.0f); }
		else if (f == NegativeY) { sc = glm::vec3(1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, 0.0f, -1.0f); ma = glm::vec3(0.0f, -1.0f, 0.0f); }
		else if (f == PositiveZ) { sc = glm::vec3(1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(0.0f, 0.0f, 1.0f); }
		else if (f == NegativeZ) { sc = glm::vec3(-1.0f, 0.0f, 0.0f); tc = glm::vec3(0.0f, -1.0f, 0.0f); ma = glm::vec3(0.0f, 0.0f, -1.0f); }
		else assert(0 && "Invalid face.");

		for (uint32_t t = 0; t < uint32_t(out_size.x); ++t) {
			for (uint32_t s = 0; s < uint32_t(out_size.x); ++s) {

				glm::vec3 N = glm::normalize(ma+ (2.0f * (s + 0.5f) / out_size.x - 1.0f) * sc+ (2.0f * (t + 0.5f) / out_size.x - 1.0f) * tc);
				glm::vec3 temp = (abs(N.z) < 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f));
				glm::vec3 TX = glm::normalize(glm::cross(N, temp));
				glm::vec3 TY = glm::cross(N, TX);

				glm::vec3 acc = glm::vec3(0.0f);
				for (uint32_t i = 0; i < uint32_t(samples); ++i) {
					//very inspired by the SampleGGX code in "Real Shading in Unreal" (https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf):


					glm::vec3 dir = make_sample();

					acc += lookup(dir.x * TX + dir.y * TY + dir.z * N);
					//acc += (dir.x * TX + dir.y * TY + dir.z * N) * 0.5f + 0.5f; //DEBUG
				}
				acc *= 1.0f / float(samples);
				if (sum_bright_directions) {
					acc += sum_bright_directions(N);
				}
				out_data.emplace_back(acc);

			}
		}
		std::cout << " done." << std::endl;
	}

	//write a single +x/-x/+y/-y/+z/-z (all stacked in a column with +x at the bottom) file for storage:
	std::cout << "Writing final rgbe png..."; std::cout.flush();
	std::vector< glm::u8vec4 > out_data_rgbe;
	out_data_rgbe.reserve(out_size.x * 6 * out_size.y);
	for (auto const& pix : out_data) {
		out_data_rgbe.emplace_back(float_to_rgbe(pix));
	}
	std::cout << " done." << std::endl;
	
	if (stbi_write_png(outpath.c_str(), out_size.x, out_size.y, 4, out_data_rgbe.data(), out_size.x * 4)) {
		std::cout << "Saved PNG successfully." << std::endl;
	}
	else {
		std::cerr << "Failed to save PNG." << std::endl;
	}
};


//------------------------------------------------------------- BRDF------------------------------------------------------------------------
//reference:https://github.com/HectorMF/BRDFGenerator/blob/master/BRDFGenerator/BRDFGenerator.cpp
float RadicalInverse_VdC(unsigned int bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

glm::vec2 Hammersley(unsigned int i, unsigned int N)
{
	return glm::vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, float roughness, glm::vec3 N)
{
	float a = roughness * roughness;

	float phi = 2.0 * M_PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	glm::vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	glm::vec3 up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
	glm::vec3 tangent = normalize(cross(up, N));
	glm::vec3 bitangent = cross(N, tangent);

	glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(float roughness, float NoV, float NoL)
{
	float ggx2 = GeometrySchlickGGX(NoV, roughness);
	float ggx1 = GeometrySchlickGGX(NoL, roughness);

	return ggx1 * ggx2;
}


glm::vec2 IntegrateBRDF(float NdotV, float roughness, unsigned int samples)
{
	glm::vec3 V;
	V.x = sqrt(1.0 - NdotV * NdotV);
	V.y = 0.0;
	V.z = NdotV;

	float A = 0.0;
	float B = 0.0;

	glm::vec3 N = glm::vec3(0.0, 0.0, 1.0);

	for (unsigned int i = 0u; i < samples; ++i)
	{
		glm::vec2 Xi = Hammersley(i, samples);
		glm::vec3 H = ImportanceSampleGGX(Xi, roughness, N);
		glm::vec3 L = normalize(2.0f * dot(V, H) * H - V);

		float NoL = glm::max(L.z, 0.0f);
		float NoH = glm::max(H.z, 0.0f);
		float VoH = glm::max(dot(V, H), 0.0f);
		float NoV = glm::max(dot(N, V), 0.0f);

		if (NoL > 0.0)
		{
			float G = GeometrySmith(roughness, NoV, NoL);

			float G_Vis = (G * VoH) / (NoH * NoV);
			float Fc = pow(1.0 - VoH, 5.0);

			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	return glm::vec2(A / float(samples), B / float(samples));
}


void PreComputeManager::BRDFCompute(const std::string &filename) {
	//Here we set up the default parameters
	int samples = 1024;
	int size = 128;
	int bits = 16;
	//std::string filename;
	std::vector<unsigned char> imageData(size * size * 3);
	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			float NoV = (y + 0.5f) * (1.0f / size);
			float roughness = (x + 0.5f) * (1.0f / size);
			glm::vec2 brdf = IntegrateBRDF(NoV, roughness, samples);
			int pixelIndex = (y * size + x) * 3; // 3 channels
			imageData[pixelIndex] = static_cast<unsigned char>(brdf.x * 255);
			imageData[pixelIndex+1] = static_cast<unsigned char>(brdf.y * 255);
			imageData[pixelIndex+2] = static_cast<unsigned char>(255);
		}

	}
	std::cout << "write brdf at " << filename << std::endl;
	stbi_write_png(filename.c_str(), size, size, 3, imageData.data(), size * 3);
}