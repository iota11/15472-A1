#pragma once
//reference:https://github.com/ixchow/15-466-ibl/tree/master/cube/blur_cube.cpp
class PreComputeManager {
	public:
		int32_t brightest = 0;
		void rgbeToRgbaFloat(uint8_t* rgbeData, float* rgbaData, size_t numPixels);
		void DiffuseCompute(const std::string& filepath, const std::string& outpath);
		void BRDFCompute(const std::string& filename);
};