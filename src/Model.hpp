#ifndef MODEL_HPP
#define MODEL_HPP
#include <glm/glm.hpp>
#include <vector>
#include "Light.hpp"
#include "ModelTriangle.h"
#include "Transform.hpp"
#include "animate.hpp"

// vec3 comparator so I can use as key in map
struct vec3Compare {
	bool operator()(const glm::vec3 &a, const glm::vec3 &b) const {
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
};

enum ModelType {
	FLAT,
	FLAT_SPECULAR,
	SMOOTH_GOURAUD,
	SMOOTH_PHONG,
	MIRROR,
	MIRROR_PHONG,
	GLASS,
	GLASS_PHONG,
	LIGHT,
};

class Model {
public:
	std::vector<ModelTriangle> triangles;
	std::string name;
	ModelType type;
	bool shadows;
	std::vector<Transformation> transformations0;
	std::vector<Transformation> transformations1;
	std::vector<Transformation> transformations2;

	Model() {}
	Model(std::string objFile, std::string mtlFile, float scale, std::string name, ModelType type, bool shadows);
	Model(Light &light) {
		triangles.emplace_back(light.position, light.position + light.uVec, light.position + light.vVec, Colour{255, 255, 255});
		triangles.emplace_back(light.position + light.uVec + light.vVec, light.position + light.uVec, light.position + light.vVec, Colour{255, 255, 255});
		name = "light";
		type = LIGHT;
		shadows = false;
	}

	void addTransformation(TransformationType type, float x = 0, float y = 0, float z = 0, float seconds = 0, int parallel = 0);

	void translate(float x, float y, float z);
	void rotate(float xDegrees, float yDegrees, float zDegrees);
	void scale(float x, float y, float z);
	// void applyTransformation();

private:
	// glm::vec3 translation {0};
	// glm::vec3 rotation {0};
	// glm::vec3 scaling {1};
	void calculateFaceNormals();
	void calculateVertexNormals();
};

#endif
