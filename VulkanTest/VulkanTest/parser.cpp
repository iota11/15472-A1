#include <iostream>
#include <fstream>

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

	struct Position {
		float x, y, z;
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


	struct SceneItem {
		int parent = 0;
		int id = 0;
		glm::mat4 trans;

		//mesh customized
		int offset = 0;
		int stride = 0;
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

		//node
		std::vector<float> childrenList;
		std::vector<float> translation;
		std::vector<float> rotation;
		std::vector<float> scale;
		int cam_id = 0;
		int mesh_id = 0;

		//driver
		int node = 0;
		std::string channel = "Null";
		std::vector<float> times;
		std::vector<float> values;
		std::string interpolation = "Null";
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
			// position
			valStart = json.find('{', valStart + 1) + 1;
			valEnd = json.find('}', valStart);
			std::string posVal = json.substr(valStart, valEnd - valStart);
			item.attributes.positionList = parseBinary32(posVal);
			//normal
			valStart = json.find('{', valEnd + 1) + 1;
			valEnd = json.find('}', valStart);
			std::string normalVal = json.substr(valStart, valEnd - valStart);
			item.attributes.normalList = parseBinary32(normalVal);

			//color
			valStart = json.find('{', valEnd + 1) + 1;
			valEnd = json.find('}', valStart);
			std::string colorVal = json.substr(valStart, valEnd - valStart);
			item.attributes.colorList = parseBinary8(colorVal);
			float itemss = 12;

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
			std::cout << "meshid: " << key << " : " << valueTest << std::endl;
			item.mesh_id = valueTest;
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
			item.channel = value;
		}
		else if (key == "times") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
			value = json.substr(valStart, valEnd - valStart);
			std::vector<float> valueTest = stringToVector(value);
			item.times = valueTest;
		}
		else if (key == "values") {
			valEnd = std::min(json.find(',', valStart), json.find('}', valStart));
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
		//data
		else if (key == "data") {
			valEnd = json.find(']', valStart) + 1;
			value = json.substr(valStart, valEnd - valStart);
			std::vector valueTest = stringToVector(value);

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

	void build() {
		unsigned int id = 0;
		for (SceneItem& item : items) {
			
			id += 1;
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
		}
		
		//arrange mesh and node transform
		for (SceneItem& item : items) {
			if (item.type == "NODE") {
				getWorldTransform(item);
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
				if (pos > json.size()) {
					break;
				}
			}
			if (pos > json.size()) {
				break;
			}
			SceneItem item{};
			while (json[pos] != '}') {
				//parse one value
				pos = parseKeyValue(item, pos, json);
				//std::cout << "stop as " << json[pos] << std::endl;
			}
			items.push_back(item);
			//std::cout << "pos is " << pos << "finish one --------------" << item.type << std::endl;
		}
		

		build();
		std::cout << "finish building items: " << items.size() << std::endl;
	}

};


