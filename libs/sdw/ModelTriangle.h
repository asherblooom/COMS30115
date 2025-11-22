#pragma once

#include <array>
#include <glm/glm.hpp>
#include "Colour.h"
#include "TexturePoint.h"

enum TriangleType {
	FLAT,
	FLAT_SPECULAR,
	SMOOTH_GOURAUD,
	SMOOTH_PHONG,
	MIRROR,
	PHONG_MIRROR
};

struct ModelTriangle {
	std::array<glm::vec3, 3> vertices{};
	std::array<TexturePoint, 3> texturePoints{};
	Colour colour{};
	glm::vec3 normal{};
	std::array<glm::vec3, 3> vertexNormals;
	TriangleType type = FLAT;
	bool shadows = true;
	std::string objName; // name of object triangle is part of

	ModelTriangle();
	ModelTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Colour trigColour);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
