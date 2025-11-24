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
	MIRROR_PHONG,
};

struct ModelTriangle {
	std::array<glm::vec3, 3> vertices{};
	std::array<TexturePoint, 3> texturePoints{};
	Colour colour{};
	glm::vec3 normal{};
	std::array<glm::vec3, 3> vertexNormals;

	ModelTriangle();
	ModelTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Colour trigColour);
	friend std::ostream &operator<<(std::ostream &os, const ModelTriangle &triangle);
};
