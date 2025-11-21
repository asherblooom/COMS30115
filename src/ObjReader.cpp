#include "ObjReader.hpp"
#include <Utils.h>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <map>

std::map<std::string, Colour> readMtlFile(std::string filename) {
	std::map<std::string, Colour> palette;

	std::ifstream inputStream(filename, std::ios::in);
	if (!inputStream) std::cerr << "ERROR: '" << filename << "' not found\n";
	std::string line;

	std::string currColourName;
	while (std::getline(inputStream, line)) {
		if (line.size() == 0) continue;
		std::vector<std::string> s = split(line, ' ');

		if (s.at(0) == "newmtl") {
			currColourName = s.at(1);
		} else if (s.at(0) == "Kd") {
			palette.insert({currColourName, Colour{int(std::stof(s.at(1)) * 255),
												   int(std::stof(s.at(2)) * 255),
												   int(std::stof(s.at(3)) * 255)}});
		}
	}

	inputStream.close();
	return palette;
}

std::vector<ModelTriangle> readObjFile(std::string objFile, std::string mtlFile, float scale) {
	std::map<std::string, Colour> palette;
	if (mtlFile != "")
		palette = readMtlFile(mtlFile);

	std::vector<ModelTriangle> triangles;
	std::vector<glm::vec3> tempVertices;
	// if no mtlFile is given, default colour is red
	Colour currColour = {255, 0, 0};

	std::ifstream inputStream(objFile, std::ios::in);
	if (!inputStream) std::cerr << "ERROR: '" << objFile << "' not found\n";
	std::string line;
	while (std::getline(inputStream, line)) {
		if (line.size() == 0) continue;
		std::vector<std::string> s = split(line, ' ');

		if (s.at(0) == "v") {
			tempVertices.emplace_back(std::stof(s.at(1)) * scale, std::stof(s.at(2)) * scale, std::stof(s.at(3)) * scale);
		}
		if (s.at(0) == "f") {
			if (s.size() > 4) std::cerr << "ERROR::OBJ: can only handle 3 indices per 'f' line!\n";
			std::vector<std::string> indices1 = split(s.at(1), '/');
			std::vector<std::string> indices2 = split(s.at(2), '/');
			std::vector<std::string> indices3 = split(s.at(3), '/');

			glm::vec3 v1, v2, v3;
			v1 = tempVertices.at(std::stoi(indices1.at(0)) - 1);
			v2 = tempVertices.at(std::stoi(indices2.at(0)) - 1);
			v3 = tempVertices.at(std::stoi(indices3.at(0)) - 1);
			triangles.emplace_back(v1, v2, v3, currColour);
		}
		if (mtlFile != "" && s.at(0) == "usemtl") {
			currColour = palette.at(s.at(1));
		}
	}
	inputStream.close();

	return triangles;
}
