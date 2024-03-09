#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm> // For remove_if
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "parser.h"

class SimpleJSONParser {
public:
	float maxPeriod = 3.0f;
	struct Position {
		float x, y, z;
	};
	struct TexCoord {
		float u, v;
	};
	struct Tangent {
		float x, y, z, w;
	};
	struct BoundingSphere {
		glm::vec3 center;
		float radius;
	};

	struct Color {
		uint8_t r, g, b, a;
	};
	struct SceneVertex {
		Position pos;
		Position normal;
		Color color;
	};
	std::vector<SceneVertex> vertexList;

	struct Attributes {
		std::vector<Position> positionList;
		std::vector<Position> normalList;
		std::vector<Color> colorList;
		std::vector<TexCoord> texCoordList;
		std::vector<Tangent> tangentList;
	};

	struct MeshNode
	{
		glm::mat4 trans;
		int mesh_id;
	};

	std::vector<MeshNode> meshNodeList;
	struct CamNode
	{
		glm::mat4 trans;
		int mesh_id;
	};

	struct PBR {
		glm::vec3 albedo = glm::vec3(1.0f,1.0f,1.0f);
		std::string albedoTex;
		float roughness;
		std::string roughtnessMap;
		float metalness;
		std::string metalnessMap;
	};

	struct Lambertian {
		glm::vec3 albedo = glm::vec3(1.0f, 1.0f, 1.0f);
		std::string albedoTex;
	};

	struct Radiance {
		std::string source;
		
	};

	struct SceneItem {
		int parent = 0;
		int id = 0;
		glm::mat4 trans;

		//mesh customized
		int offset = 0;
		int stride = 0;

		BoundingSphere bs;
		//all
		std::string type = "Null";
		std::string name = "Null";
		//root
		std::vector<float> roots;
		//camera
		float aspect = 0;
		float vfov = 0;
		float near = 0;
		float far = 0;
		//mesh
		std::string topology = "Null";
		int count = 0;
		Attributes attributes;
		int material_id = 0;

		//node
		std::vector<float> childrenList;
		std::vector<float> translation = {0, 0, 0};
		std::vector<float> rotation = {0,0,0,1};
		std::vector<float> scale = {1,1,1};
		int cam_id = 0;
		int mesh_id = 0;
		int env_id = 0;

		//driver
		int node = 0;
		std::string channel = "Null";
		std::vector<float> times;
		std::vector<float> values;
		std::string interpolation = "Null";

		//material
		std::string normalMap;
		std::string displacementMap;
		PBR pbrAttrib;
		Lambertian lambertAttrib;
		bool isPBR = false;
		bool isLambert = false;
		bool isEnv = false;
		bool isSimple = false;
		bool isMirror = false;


		//environment
		Radiance radiance;
	};

	std::vector<SceneItem> items;



	/// <summary>
	/// ///////////////////////////////// ------------------------------------------------------------------------------------------------- -helper ---------------------------------------------------------------------------------------
	/// </summary>
	std::vector<float> stringToVector(std::string input) {
		std::vector<float> numbers;

		// Remove square brackets
		input.erase(remove_if(input.begin(), input.end(), [](char c) { return c == '[' || c == ']'; }), input.end());

		// Use stringstream to split the string by commas and convert to integers
		std::stringstream ss(input);
		std::string item;
		while (getline(ss, item, ',')) {
			numbers.push_back(std::stod(item));
		}
		return numbers;
	}


	std::vector<Position> readBinary32(std::string filePath, const size_t s, size_t o) {
		// Open the file for input in binary mode
		std::ifstream file(filePath, std::ios::binary);
		std::vector<Position> positions;

		if (!file) {
			std::cerr << "Cannot open file: " << filePath << std::endl;
			return positions;
		}

		const size_t stride = s;
		size_t offset = o;
		while (file) {
			// Seek to the current offset
			file.seekg(offset);
			Position pos;
			// Read the position data
			file.read(reinterpret_cast<char*>(&pos), sizeof(Position));
			if (file) { // If the read was successful, add the position to the vector
				positions.push_back(pos);
				offset += stride;
			}
		}

		file.close();
		return positions;
	}
	std::vector<Tangent> readBinary32x4(std::string filePath, const size_t s, size_t o) {
		// Open the file for input in binary mode
		std::ifstream file(filePath, std::ios::binary);
		std::vector<Tangent> tangents;

		if (!file) {
			std::cerr << "Cannot open file: " << filePath << std::endl;
			return tangents;
		}

		const size_t stride = s;
		size_t offset = o;
		while (file) {
			// Seek to the current offset
			file.seekg(offset);
			Tangent tant;
			// Read the position data
			file.read(reinterpret_cast<char*>(&tant), sizeof(Tangent));
			if (file) { // If the read was successful, add the position to the vector
				tangents.push_back(tant);
				offset += stride;
			}
		}

		file.close();
		return tangents;
	}
	std::vector<TexCoord> readBinary32x2(std::string filePath, const size_t s, size_t o) {
		// Open the file for input in binary mode
		std::ifstream file(filePath, std::ios::binary);
		std::vector<TexCoord> texcoords;

		if (!file) {
			std::cerr << "Cannot open file: " << filePath << std::endl;
			return texcoords;
		}

		const size_t stride = s;
		size_t offset = o;
		while (file) {
			// Seek to the current offset
			file.seekg(offset);
			TexCoord texC;
			// Read the position data
			file.read(reinterpret_cast<char*>(&texC), sizeof(TexCoord));
			if (file) { // If the read was successful, add the position to the vector
				texcoords.push_back(texC);
				offset += stride;
			}
		}

		file.close();
		return texcoords;
	}

	std::vector<Color> readBinary8(std::string filePath, const size_t s, size_t o) {
		std::vector<Color> colors;
		std::ifstream file(filePath, std::ios::binary);

		if (!file) {
			std::cerr << "Cannot open file: " << filePath << std::endl;
			return colors; // Return empty vector on failure
		}
		const size_t stride = s;
		size_t offset = o;
		while (file) {
			file.seekg(offset);
			Color color;
			file.read(reinterpret_cast<char*>(&color), sizeof(Color));
			if (file) { // If the read was successful, add the color to the vector
				colors.push_back(color);
				offset += stride;
			}
		}

		file.close();
		// Output the positions to verify
		for (const auto& c : colors) {
			//std::cout << "color: (" <<(int)c.r << ", " << (int)c.g << ", " << (int)c.b << ")" << std::endl;
		}
		return colors;
	}


	std::vector<Position> parseBinary32(std::string& text) {
		std::string path = "s72-main/examples/";
		int valStart = text.find(':') + 1;
		int valEnd = text.find(',');
		std::string filepath = text.substr(valStart, valEnd - valStart);
		filepath.erase(std::remove(filepath.begin(), filepath.end(), '\"'), filepath.end());
		filepath = path + filepath;
		//offset
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t offset = std::stoi(text.substr(valStart, valEnd - valStart));

		//stride
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t stride = std::stoi(text.substr(valStart, valEnd - valStart));
		return readBinary32(filepath, stride, offset);
	}
	std::vector<Tangent> parseBinary32x4(std::string& text) {
		std::string path = "s72-main/examples/";
		int valStart = text.find(':') + 1;
		int valEnd = text.find(',');
		std::string filepath = text.substr(valStart, valEnd - valStart);
		filepath.erase(std::remove(filepath.begin(), filepath.end(), '\"'), filepath.end());
		filepath = path + filepath;
		//offset
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t offset = std::stoi(text.substr(valStart, valEnd - valStart));

		//stride
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t stride = std::stoi(text.substr(valStart, valEnd - valStart));
		return readBinary32x4(filepath, stride, offset);
	}

	std::vector<TexCoord> parseBinary32x2(std::string& text) {
		std::string path = "s72-main/examples/";
		int valStart = text.find(':') + 1;
		int valEnd = text.find(',');
		std::string filepath = text.substr(valStart, valEnd - valStart);
		filepath.erase(std::remove(filepath.begin(), filepath.end(), '\"'), filepath.end());
		filepath = path + filepath;
		//offset
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t offset = std::stoi(text.substr(valStart, valEnd - valStart));

		//stride
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t stride = std::stoi(text.substr(valStart, valEnd - valStart));
		return readBinary32x2(filepath, stride, offset);
	}
	std::vector<Color> parseBinary8(std::string& text) {
		std::string path = "s72-main/examples/";
		int valStart = text.find(':') + 1;
		int valEnd = text.find(',');
		std::string filepath = text.substr(valStart, valEnd - valStart);
		filepath.erase(std::remove(filepath.begin(), filepath.end(), '\"'), filepath.end());
		filepath = path + filepath;
		//offset
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t offset = std::stoi(text.substr(valStart, valEnd - valStart));

		//stride
		valStart = text.find(':', valEnd) + 1;
		valEnd = text.find(',', valStart);
		size_t stride = std::stoi(text.substr(valStart, valEnd - valStart));
		return readBinary8(filepath, stride, offset);
	}

	int parseKeyValue(SceneItem& item, int pos, std::string json) {
		// Parse key
		auto keyStart = json.find('"', pos) + 1; // Skip opening quote
		auto keyEnd = json.find('"', keyStart);
		auto key = json.substr(keyStart, keyEnd - keyStart);
		std::cout << "read :" << key << std::endl;

		// Move pos to value start
		pos = json.find(':', keyEnd) + 1;
		// Parse value
		int valEnd = pos;
		std::string value;
		int valStart = pos;
		//general
		if (key == "type") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
			item.type = value;
		}
		else if (key == "name") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			item.name = value;
		}

		//mesh
		else if (key == "topology") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());//
			item.topology = value;
		}
		else if (key == "count") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			int num = std::stoi(value);//
			item.count = num;

		}
		else if (key == "attributes") {
			int cnt = 0;
			char test = json[valStart];
			while (json[valEnd] != '}') {
				cnt++;
				if (cnt > 5) break;
				valStart = json.find('\"', valEnd);
				valEnd = json.find('\"', valStart+1);
				auto key2 = json.substr(valStart, valEnd - valStart);
				key2.erase(std::remove(key2.begin(), key2.end(), '\"'), key2.end());
				std::cout << "   key2 is " << key2 << std::endl;
				if (key2 == "POSITION") {
					// position
					valStart = json.find('{', valStart + 1) + 1;
					valEnd = json.find('}', valStart);
					std::string posVal = json.substr(valStart, valEnd - valStart);
					item.attributes.positionList = parseBinary32(posVal);
				}
				
				else if (key2 == "NORMAL") {
					//normal
					valStart = json.find('{', valEnd + 1) + 1;
					valEnd = json.find('}', valStart);
					std::string normalVal = json.substr(valStart, valEnd - valStart);
					item.attributes.normalList = parseBinary32(normalVal);
				}
				else if (key2 == "TANGENT") {
					//Tangent
					valStart = json.find('{', valEnd + 1) + 1;
					valEnd = json.find('}', valStart);
					std::string tangentVal = json.substr(valStart, valEnd - valStart);
					item.attributes.tangentList = parseBinary32x4(tangentVal);
				}
				else if (key2 == "TEXCOORD") {
					// texcoord
					valStart = json.find('{', valEnd + 1) + 1;
					valEnd = json.find('}', valStart);
					std::string texcoodVal = json.substr(valStart, valEnd - valStart);
					item.attributes.texCoordList = parseBinary32x2(texcoodVal);
				}
				else if (key2 == "COLOR") {
					//color
					valStart = json.find('{', valEnd + 1) + 1;
					valEnd = json.find('}', valStart);
					std::string colorVal = json.substr(valStart, valEnd - valStart);
					item.attributes.colorList = parseBinary8(colorVal);
					
				}
				
				valEnd += 1; //skip this"}"
				while ((json[valEnd] == ' ')  || (json[valEnd] == '\t') || (json[valEnd] == '\n') || (json[valEnd] == ',')) {
					//std::cout << "skip the " << json[valEnd] << " ------------------"<<std::endl;
					//char test = json[valEnd];
					valEnd++;
				}
			}

			valEnd = json.find('}', valEnd) +1;
			float t = 1;
		}
		else if (key == "material") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			int num = std::stoi(value);//
			item.material_id = num;
		}


		//node 
		else if (key == "translation") {
			valEnd = json.find(']', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			std::vector valueTest = stringToVector(value);
			item.translation = valueTest;
		}
		else if (key == "rotation") {
			valEnd = json.find(']', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			std::vector valueTest = stringToVector(value);
			item.rotation = valueTest;
		}
		else if (key == "scale") {
			valEnd = json.find(']', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			std::vector valueTest = stringToVector(value);
			item.scale = valueTest;
		}
		else if (key == "mesh") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			int valueTest = std::stoi(value);
			//std::cout << "meshid: " << key << " : " << valueTest << std::endl;
			item.mesh_id = valueTest;
		}

		else if (key == "environment") {
			if (item.type == "NODE") {
				valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
				value = json.substr(valStart, valEnd - valStart);
				int valueTest = std::stoi(value);
				std::cout << "environmentid: " << key << " : " << valueTest << std::endl;
				item.env_id = valueTest;
			}
			else {
				item.isEnv = true;
				valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			}
		}

		else if (key == "children") {
			valEnd = json.find('}', valStart);
			value = json.substr(valStart, valEnd - valStart);
			std::vector<float> valueTest = stringToVector(value);
			std::cout << "hi there: " << value << " : " << valueTest.size() << std::endl;
			item.childrenList = valueTest;
		}

		else if (key == "camera") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			int valueTest = std::stoi(value);
			item.cam_id = valueTest;
		}
		//camera
		else if (key == "perspective") {
			valEnd = json.find('}', valStart);
			value = json.substr(valStart, valEnd - valStart);

			valStart = json.find(':', valStart) + 1;
			int valEndTemp = json.find(',', valStart);
			float aspectVal = std::stof(json.substr(valStart, valEndTemp - valStart));


			valStart = valEndTemp;
			valStart = json.find(':', valStart) + 1;
			valEndTemp = json.find(',', valStart);
			float vfovVal = std::stof(json.substr(valStart, valEndTemp - valStart));


			valStart = valEndTemp;
			valStart = json.find(':', valStart) + 1;
			valEndTemp = json.find(',', valStart);
			float nearVal = std::stof(json.substr(valStart, valEndTemp - valStart));

			valStart = valEndTemp;
			valStart = json.find(':', valStart) + 1;
			float farVal = std::stof(json.substr(valStart));

			item.aspect = aspectVal;
			item.far = farVal;
			item.near = nearVal;
			item.vfov = vfovVal;
		}

		//driver
		else if (key == "node") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			int valueTest = std::stoi(value);
			item.node = valueTest;
		}

		else if (key == "channel") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());

			item.channel = value;
		}
		else if (key == "times") {
			valEnd = json.find(']', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			std::vector<float> valueTest = stringToVector(value);
			std::cout << "hi there: " << key << " : " << valueTest.size() << std::endl;
			item.times = valueTest;
		}
		else if (key == "values") {
			valEnd = json.find(']', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			std::vector<float> valueTest = stringToVector(value);
			item.values = valueTest;
		}
		else if (key == "interpolation") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			item.interpolation = value;
		}
		//scene
		else if (key == "roots") {
			valEnd = json.find('}', valStart);
			value = json.substr(valStart, valEnd - valStart);
			std::vector<float> valueTest = stringToVector(value);
			item.roots = valueTest;
			//std::cout << "hi there: " << key << " : " << valueTest [0] << std::endl;

		}


		//materials
		else if (key == "normalMap") {
			valEnd = json.find('}', valStart);
			valStart = json.find(':', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
			std::cout << "normal map is " << value << std::endl;
			item.normalMap = value;

		}

		else if (key == "displacementMap") {
			valEnd = json.find('}', valStart);
			valStart = json.find(':', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
			std::cout << "displacement map is " << value << std::endl;
			item.normalMap = value;
		}

		else if (key == "pbr") {
			item.isPBR = true;
			//albedo
			valStart = json.find(':', valStart) + 1;
			while (json[valStart] == ' ') valStart += 1;
			if (json[valStart] == '[') {
				valStart += 1;
				valEnd = json.find(']', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				std::vector valueTest = stringToVector(value);
				item.pbrAttrib.albedo = glm::vec3(valueTest[0], valueTest[1], valueTest[2]);
			}
			else if (json[valStart] == '{') {
				valEnd = json.find('}', valStart);
				valStart = json.find(':', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
				std::cout << "pbr albedo map is " << value << std::endl;
				item.pbrAttrib.albedoTex = value;
			}
			//roughness
			valStart = json.find(':', valStart) + 1;
			while (json[valStart] == ' ') valStart += 1;
			if (json[valStart] == '{') {
				valEnd = json.find('}', valStart);
				valStart = json.find(':', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
				std::cout << "pbr roughness map is " << value << std::endl;
				item.pbrAttrib.roughtnessMap = value;
			}
			else {
				valEnd = json.find(',', valStart);
				value = json.substr(valStart, valEnd - valStart);
				float valueTest = std::stof(value);
				//std::cout << "meshid: " << key << " : " << valueTest << std::endl;
				item.pbrAttrib.roughness = valueTest;
			}

			//metalness
			valStart = json.find(':', valStart) + 1;
			while (json[valStart] == ' ') valStart += 1;
			if (json[valStart] == '{') {
				valEnd = json.find('}', valStart);
				valStart = json.find(':', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
				std::cout << "pbr metal map is " << value << std::endl;
				item.pbrAttrib.metalnessMap = value;
			}
			else {
				valEnd = json.find(',', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				float valueTest = std::stof(value);
				//std::cout << "meshid: " << key << " : " << valueTest << std::endl;
				item.pbrAttrib.metalness = valueTest;
			}

			valEnd = json.find('}', valStart) + 1;
		}


		else if (key == "lambertian") {
			item.isLambert = true;

			//albedo
			valStart = json.find(':', valStart) + 1;
			while (json[valStart] == ' ') valStart += 1;
			if (json[valStart] == '[') {
				valStart += 1;
				valEnd = json.find(']', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				std::vector valueTest = stringToVector(value);
				item.lambertAttrib.albedo = glm::vec3(valueTest[0], valueTest[1], valueTest[2]);
			}
			else if (json[valStart] == '{') {
				valEnd = json.find('}', valStart);
				valStart = json.find(':', valStart) + 1;
				value = json.substr(valStart, valEnd - valStart);
				value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
				std::cout << "pbr albedo map is " << value << std::endl;
				item.pbrAttrib.albedoTex = value;
			}
		}

		else if (key == "mirror") {
			item.isMirror = true;
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
		}
		else if (key == "simple") {
			item.isMirror = true;
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
		}


		//environemnt
		else if (key == "radiance") {
			valEnd = json.find(',', valStart);
			valStart = json.find(':', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
			std::cout << "radiance map is " << value << std::endl;
			item.radiance.source = value;
			valEnd = json.find('}', valStart) + 1;
		}
		//loop to the next value
		while ((json[valEnd] == ' ') || (json[valEnd] == '\t') || (json[valEnd] == '\n') || (json[valEnd] == ',')) {
			valEnd++;
		}

		pos = valEnd;
		return pos;
	}



	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	glm::mat4 calculateModelMatrix(const std::vector<float>& translation, const std::vector<float>& rotation, std::vector<float>& scale) {
		// Create a translation matrix
		float x = translation[0];
		float y = translation[1];
		float z = translation[2];
		glm::vec3 translate(x,y,z);
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translate);
		glm::quat rot = glm::quat(rotation[3], rotation[0], rotation[1], rotation[2]);
		glm::mat4 rotationMatrix = glm::toMat4(rot);

		// Create a scale matrix
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale[0], scale[1], scale[2]));

		glm::mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

		return modelMatrix;
	}
	void getWorldTransform(SceneItem& item) {
		SceneItem* curItem = &item;
		while (true) {
			if (curItem->parent <= 0) {
				std::cout << "error node " << curItem->name << std::endl;
				break;
			}
			if (items[curItem->parent - 1].type == "SCENE") {
				break;
			}

			item.trans = items[curItem->parent - 1].trans * item.trans;
			curItem = &items[curItem->parent - 1];
		}
	}

	int findInterval(std::vector<float> & frames, float time) {
		int pos = 0;
		for (int i = 0; i < frames.size(); i++) {
			pos = i;
			if (frames[i] > time) {
				
				break;
			}
		}
		return pos;
	}

	glm::quat slerpQ(const glm::quat& q1, const glm::quat& q2, float t) {
		float cosTheta = glm::dot(q1, q2);


		if (cosTheta < 0.0f) {
			return slerp(q1, -q2, t);
		}

		if (cosTheta > 0.9995f) {
			return glm::normalize(glm::lerp(q1, q2, t));
		}
		else {
			float angle = acos(cosTheta);
			return (glm::sin((1.0f - t) * angle) * q1 + glm::sin(t * angle) * q2) / glm::sin(angle);
		}
	}

	glm::vec3 vectorSlerp(const glm::vec3& v1, const glm::vec3& v2, float t) {
		glm::vec3 unitV1 = glm::normalize(v1);
		glm::vec3 unitV2 = glm::normalize(v2);

		float dotProduct = glm::dot(unitV1, unitV2);

		dotProduct = glm::clamp(dotProduct, -1.0f, 1.0f);

		float omega = std::acos(dotProduct);

		if (std::abs(omega) < 1e-4) {
			return glm::normalize(glm::mix(unitV1, unitV2, t));
		}

		float sinOmega = std::sin(omega);
		float a = std::sin((1 - t) * omega) / sinOmega;
		float b = std::sin(t * omega) / sinOmega;

		return glm::normalize((a * v1) + (b * v2));
	}
	/// ///////////////////////////////// ------------------------------------------------------------------------------------------------- -bouding sphere ---------------------------------------------------------------------------------------


	BoundingSphere calculateBoundingSphere(const std::vector<Position>& vertices, const glm::mat4& globalTransform) {
		// Transform vertices
		std::vector<glm::vec3> transformedVertices;
		for (const auto& v : vertices) {
			glm::vec4 transformed = globalTransform * glm::vec4(v.x, v.y,v.z, 1.0f);
			transformedVertices.push_back(glm::vec3(transformed));
		}

		// Calculate center
		glm::vec3 center(0.0f);
		for (const auto& v : transformedVertices) {
			center += v;
		}
		center /= static_cast<float>(transformedVertices.size());

		// Calculate radius
		float radius = 0.0f;
		for (const auto& v : transformedVertices) {
			float distance = glm::distance(center, v);
			radius = std::max(radius, distance);
		}

		return BoundingSphere{ center, radius };
	}

	//call each frame----------------------------------------------------------------------------------------------------------------------------------------------------------
	void update(float globalTime) {
		globalTime = std::fmod(globalTime, maxPeriod);
		for (SceneItem& item : items) {
			if (item.type == "DRIVER") {
				if (item.channel == "translation") {
					int pos = findInterval(item.times, globalTime);
					float time_s = item.times[pos - 1];
					float time_e = item.times[pos];
					if (globalTime >= time_e) {
						globalTime = time_e;
					}
					float ratio = (globalTime - time_s) / (time_e - time_s);
					glm::vec3 value_s(item.values[3 * (pos - 1)], item.values[3 * (pos - 1) + 1], item.values[3 * (pos - 1) + 2]);
					glm::vec3 value_e(item.values[3 * (pos)], item.values[3 * (pos) + 1], item.values[3 * (pos) + 2]);
					
					glm::vec3 value;
					if (item.interpolation == "LINEAR") {
						value = value_s + ratio * (value_e - value_s);
					}
					else if (item.interpolation == "STEP") {
						value = value_s;
					}
					else if (item.interpolation == "SLERP") {
						vectorSlerp(value_s, value_e, ratio);
					}
					else {
						value = value_s;
					}
					items[item.node - 1].translation = { value.x, value.y, value.z };
				}
				if (item.channel == "scale") {
					int pos = findInterval(item.times, globalTime);
					float time_s = item.times[pos - 1];
					float time_e = item.times[pos];
					if (globalTime >= time_e) {
						globalTime = time_e;
					}
					float ratio = (globalTime - time_s) / (time_e - time_s);
					glm::vec3 value_s(item.values[3 * (pos - 1)], item.values[3 * (pos - 1) + 1], item.values[3 * (pos - 1) + 2]);
					glm::vec3 value_e(item.values[3 * (pos)], item.values[3 * (pos)+1], item.values[3 * (pos)+2]);

					glm::vec3 value;
					if (item.interpolation == "LINEAR") {
						value = value_s + ratio * (value_e - value_s);
					}
					else if (item.interpolation == "STEP") {
						value = value_s;
					}
					else if (item.interpolation == "SLERP") {
						vectorSlerp(value_s, value_e, ratio);
					}
					else {
						value = value_s;
					}
					items[item.node - 1].scale = { value.x, value.y, value.z };
				}
				if (item.channel == "rotation") {
					int pos = findInterval(item.times, globalTime);
					float time_s = item.times[pos - 1];
					float time_e = item.times[pos];
					if (globalTime >= time_e) {
						globalTime = time_e;
					}
					float ratio = (globalTime - time_s) / (time_e - time_s);
					glm::quat value_s(item.values[4 * (pos - 1)], item.values[4 * (pos - 1) + 1], item.values[4 * (pos - 1) + 2], item.values[4 * (pos - 1) + 3]);
					glm::quat value_e(item.values[4 * (pos)], item.values[4 * (pos)+1], item.values[4 * (pos)+2],  item.values[4 * (pos) + 3]);

					glm::quat value = value_s;
					if (item.interpolation == "LINEAR") {
						value = value_s + ratio * (value_e - value_s);
					}
					else if (item.interpolation == "STEP") {
						value = value_s;
					}
					else if (item.interpolation == "SLERP") {
						slerpQ(value_s, value_e, ratio);
					}
					else {
						value = value_s;
					}
					items[item.node - 1].rotation = { value.w, value.x, value.y, value.z};
				}
			}
		}

		rebuild();
	}

	void rebuild() {
		for (SceneItem& item : items) {
			if (item.type == "NODE") {
				item.trans = calculateModelMatrix(item.translation, item.rotation, item.scale);
			}
		}

		//arrange mesh and node transform
		for (SceneItem& item : items) {
			if (item.type == "NODE") {
				getWorldTransform(item);
				if (item.mesh_id > 0) {
					item.bs = calculateBoundingSphere(items[item.mesh_id - 1].attributes.positionList, item.trans);
				}
			}
		}
	}

	void build() {
		unsigned int id = 0;
		for (SceneItem& item : items) {
			
			id += 1;
			item.id = id;
			if (item.type == "SCENE") {
				for (float i : item.roots) {
					items[i - 1].parent = id;
				}
			}
			if (item.type == "NODE") {
				for (float i : item.childrenList) {
					items[i - 1].parent = id;
				}

				item.trans = calculateModelMatrix(item.translation, item.rotation, item.scale);
			}
			if (item.type == "MESH") {
				item.offset = vertexList.size();
				item.stride = item.count;
				for (int i = 0; i < item.count; i++) {
					SceneVertex sv;
					sv.pos = item.attributes.positionList[i];
					sv.normal = item.attributes.normalList[i];
					sv.color = item.attributes.colorList[i];
					vertexList.push_back(sv);
				}
			}

			if (item.type == "DRIVER") {
				float maxtime = item.times[item.times.size() - 1];
				if (maxtime > maxPeriod) maxPeriod = maxtime;
				std::cout << maxtime << std::endl;
			}

			if (item.type == "MATERIAL") {
				
			}
			if (item.type == "ENVIRONMENT") {
				
			}
		}
		
		//arrange mesh and node transform
		for (SceneItem& item : items) {
			if (item.type == "NODE") {
				getWorldTransform(item);

				if (item.mesh_id > 0) {
					item.bs = calculateBoundingSphere(items[item.mesh_id - 1].attributes.positionList, item.trans);
				}
			}
			
		}
		

	}

	void parse(const std::string& json) {
		std::map<std::string, std::string> result;
		size_t pos = 0;
		int cnt = 0;
		int sizesdss = json.size();
		while (true) {
			while (json[pos] != '{') {
				++pos;
				if (pos >= json.size()) {
					break;
				}
			}
			if (pos >= json.size()) {
				break;
			}
			SceneItem item{};
			while (json[pos] != '}') {
				//parse one value
				pos = parseKeyValue(item, pos, json);
				//std::cout << "stop as " << json[pos] << std::endl;
				cnt++;
				//if (cnt > 10000) break;
			}
			items.push_back(item);
			std::cout << "pos is " << pos << "finish one --------------" << item.type << std::endl;
			cnt++;
			//if (cnt > 10000) break;
		}
		

		build();
		std::cout << "finish building items: " << items.size() << std::endl;

		
	}

};


