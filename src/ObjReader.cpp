#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "ModelTriangle.h"
#include "Utils.h"

std::vector<ModelTriangle> readObjFile(std::string filename, float scale) {
	std::vector<ModelTriangle> triangles;
	std::vector<int> vertexIndices;
	// std::vector<int> uvIndices;
	// std::vector<int> normalIndices;
	std::vector<glm::vec3> tempVertices;
	// std::vector<glm::vec2> tempUvs;
	// std::vector<glm::vec3> tempNormals;

	std::ifstream inputStream(filename, std::ios::in);
	if (!inputStream) std::cerr << "ERROR: '" << filename << "' not found\n";
	std::string line;
	while (std::getline(inputStream, line)) {
		if (line.size() == 0) continue;
		switch (line[0]) {
			case '#':
				continue;
			case 'v': {
				std::vector<std::string> s = split(line, ' ');
				// if (line[1] == 't')
				// 	tempUvs.emplace_back(std::stof(s.at(1)), std::stof(s.at(2)));
				// else if (line[1] == 'n')
				// 	tempNormals.emplace_back(std::stof(s.at(1)), std::stof(s.at(2)), std::stof(s.at(3)));
				// else
				tempVertices.emplace_back(std::stof(s.at(1)) * scale, std::stof(s.at(2)) * scale, std::stof(s.at(3)) * scale);
				break;
			}
			case 'f': {
				auto s = split(line, ' ');
				if (s.size() > 4) std::cerr << "ERROR::OBJ: can only handle 3 indices per 'f' line!\n";
				for (int i = 1; i < s.size(); i++) {
					std::vector<std::string> indices = split(s.at(i), '/');

					vertexIndices.push_back(std::stoi(indices.at(0)));
					// if (indices.size() > 1 && indices.at(1) != "")
					// 	uvIndices.emplace_back(std::stof(indices.at(1)));
					// if (indices.size() > 2 && indices.at(2) != "")
					// 	normalIndices.emplace_back(std::stof(indices.at(2)));
				}
				break;
			}
			default:
				continue;
		}
	}
	inputStream.close();

	for (int i = 0; i < vertexIndices.size(); i += 3) {
		glm::vec3 i1, i2, i3;
		i1 = tempVertices.at(vertexIndices.at(i) - 1);
		i2 = tempVertices.at(vertexIndices.at(i + 1) - 1);
		i3 = tempVertices.at(vertexIndices.at(i + 2) - 1);
		triangles.emplace_back(i1, i2, i3, Colour{255, 255, 255});
	}
	//
	// for (int i = 0; i < normalIndices.size(); i += 3) {
	// 	glm::vec3 normals{temp_normals.at(normalIndices.at(i) - 1),
	// 					  temp_normals.at(normalIndices.at(i + 1) - 1),
	// 					  temp_normals.at(normalIndices.at(i + 2) - 1)};
	// 	data.normals.push_back(normals);
	// }
	// for (int i = 0; i < uvIndices.size(); i += 2) {
	// 	glm::vec2 uvs{temp_uvs.at(uvIndices.at(i) - 1),
	// 				  temp_uvs.at(uvIndices.at(i + 1) - 1)};
	// 	data.uvs.push_back(uvs);
	// }
	return triangles;
}
