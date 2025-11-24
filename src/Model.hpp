#ifndef MODEL_HPP
#define MODEL_HPP
#include "ModelTriangle.h"
#include "Transform.hpp"
#include "ObjReader.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <map>

// vec3 comparator so I can use as key in map
struct vec3Compare {
    bool operator()(const glm::vec3 &a, const glm::vec3 &b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

class Model {
public:
    std::vector<ModelTriangle> triangles;
    std::string name;
    TriangleType type;
    bool shadows;

    Model(){}
    Model(std::string objFile, std::string mtlFile, float scale, std::string name, TriangleType type, bool shadows);

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